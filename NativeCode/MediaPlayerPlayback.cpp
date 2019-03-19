//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.h"
#include "MediaPlayerPlayback.h"
#include "MediaHelpers.h"

using namespace Microsoft::WRL;
using namespace ABI::Windows::Graphics::DirectX::Direct3D11;
using namespace ABI::Windows::Media;
using namespace ABI::Windows::Media::Core;
using namespace ABI::Windows::Media::Playback;
using namespace Windows::Foundation;

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::CreateMediaPlayback(
    UnityGfxRenderer apiType, 
    IUnityInterfaces* pUnityInterfaces,
    StateChangedCallback fnCallback,
    IMediaPlayerPlayback** ppMediaPlayback)
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::CreateMediaPlayback()");

    NULL_CHK(pUnityInterfaces);
    NULL_CHK(fnCallback);
    NULL_CHK(ppMediaPlayback);

    *ppMediaPlayback = nullptr;

    if (apiType == kUnityGfxRendererD3D11)
    {
        IUnityGraphicsD3D11* d3d = pUnityInterfaces->Get<IUnityGraphicsD3D11>();
        NULL_CHK_HR(d3d, E_INVALIDARG);

        ComPtr<CMediaPlayerPlayback> spMediaPlayback(nullptr);
        IFR(MakeAndInitialize<CMediaPlayerPlayback>(&spMediaPlayback, fnCallback, d3d->GetDevice()));

        *ppMediaPlayback = spMediaPlayback.Detach();
    }
    else
    {
        IFR(E_INVALIDARG);
    }

    return S_OK;
}


_Use_decl_annotations_
CMediaPlayerPlayback::CMediaPlayerPlayback()
    : m_d3dDevice(nullptr)
    , m_mediaDevice(nullptr)
    , m_fnStateCallback(nullptr)
    , m_mediaPlayer(nullptr)
    , m_mediaPlaybackSession(nullptr)
    , m_primaryTexture(nullptr)
    , m_primaryTextureSRV(nullptr)
    , m_primarySharedHandle(INVALID_HANDLE_VALUE)
    , m_primaryMediaTexture(nullptr)
    , m_primaryMediaSurface(nullptr)
{
}

_Use_decl_annotations_
CMediaPlayerPlayback::~CMediaPlayerPlayback()
{
    ReleaseTextures();

    ReleaseMediaPlayer();

    MFUnlockDXGIDeviceManager();

    ReleaseResources();
}


_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::RuntimeClassInitialize(
    StateChangedCallback fnCallback,
    ID3D11Device* pDevice)
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::RuntimeClassInitialize()");

    NULL_CHK(fnCallback);
    NULL_CHK(pDevice);

    // ref count passed in device
    ComPtr<ID3D11Device> spDevice(pDevice);

    // make sure creation of the device is on the same adapter
    ComPtr<IDXGIDevice> spDXGIDevice;
    IFR(spDevice.As(&spDXGIDevice));
    
    ComPtr<IDXGIAdapter> spAdapter;
    IFR(spDXGIDevice->GetAdapter(&spAdapter));

    // create dx device for media pipeline
    ComPtr<ID3D11Device> spMediaDevice;
    IFR(CreateMediaDevice(spAdapter.Get(), &spMediaDevice));

    // lock the shared dxgi device manager
    // will keep lock open for the life of object
    //     call MFUnlockDXGIDeviceManager when unloading
    UINT uiResetToken;
    ComPtr<IMFDXGIDeviceManager> spDeviceManager;
    IFR(MFLockDXGIDeviceManager(&uiResetToken, &spDeviceManager));

    // associtate the device with the manager
    IFR(spDeviceManager->ResetDevice(spMediaDevice.Get(), uiResetToken));

    // create media plyaer object
    IFR(CreateMediaPlayer());

    m_fnStateCallback = fnCallback;
    m_d3dDevice.Attach(spDevice.Detach());
    m_mediaDevice.Attach(spMediaDevice.Detach());

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::CreatePlaybackTexture(
    UINT32 width,
    UINT32 height, 
    void** ppvTexture)
{
    NULL_CHK(ppvTexture);

    if (width < 1 || height < 1)
        IFR(E_INVALIDARG);

    *ppvTexture = nullptr;

    // create the video texture description based on texture format
    m_textureDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_B8G8R8A8_UNORM, width, height);
    m_textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    m_textureDesc.MipLevels = 1;
    m_textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    m_textureDesc.Usage = D3D11_USAGE_DEFAULT;

    IFR(CreateTextures());

    ComPtr<ID3D11ShaderResourceView> spSRV;
    IFR(m_primaryTextureSRV.CopyTo(&spSRV));

    *ppvTexture = spSRV.Detach();

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::LoadContent(
    LPCWSTR pszContentLocation)
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::LoadContent()");

    // create the media source for content (fromUri)
    ComPtr<IMediaSource2> spMediaSource2;
    IFR(CreateMediaSource(pszContentLocation, &spMediaSource2));

    ComPtr<IMediaPlaybackItem> spPlaybackItem;
    IFR(CreateMediaPlaybackItem(spMediaSource2.Get(), &spPlaybackItem));

    ComPtr<IMediaPlaybackSource> spMediaPlaybackSource;
    IFR(spPlaybackItem.As(&spMediaPlaybackSource));

    ComPtr<IMediaPlayerSource2> spMediaPlayerSource;
    IFR(m_mediaPlayer.As(&spMediaPlayerSource));
    IFR(spMediaPlayerSource->put_Source(spMediaPlaybackSource.Get()));

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::Play()
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::Play()");

    if (nullptr != m_mediaPlayer)
    {
        MediaPlayerState state;
        LOG_RESULT(m_mediaPlayer->get_CurrentState(&state));

        IFR(m_mediaPlayer->Play());
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::Pause()
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::Pause()");

    if (nullptr != m_mediaPlayer)
    {
        IFR(m_mediaPlayer->Pause());
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::Stop()
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::Stop()");

    if (nullptr != m_mediaPlayer)
    {
        ComPtr<IMediaPlayerSource2> spMediaPlayerSource;
        IFR(m_mediaPlayer.As(&spMediaPlayerSource));
        IFR(spMediaPlayerSource->put_Source(nullptr));
    }

    return S_OK;
}

HRESULT CMediaPlayerPlayback::GetPosition(LONGLONG* position)
{
	Log(Log_Level_Info, L"CMediaPlayerPlayback::GetPosition()");

	if (nullptr != m_mediaPlayer)
	{
		m_mediaPlayer->get_Position( ((ABI::Windows::Foundation::TimeSpan*) position) );
	}
	return S_OK;
}

HRESULT CMediaPlayerPlayback::GetDuration(LONGLONG* duration)
{
	Log(Log_Level_Info, L"CMediaPlayerPlayback::GetPosition()");

	if (nullptr != m_mediaPlayer)
	{
		m_mediaPlaybackSession->get_NaturalDuration(((ABI::Windows::Foundation::TimeSpan*) duration));
	}
	return S_OK;
}

HRESULT CMediaPlayerPlayback::SetPosition(LONGLONG position)
{
	Log(Log_Level_Info, L"CMediaPlayerPlayback::SetPosition()");

	if (nullptr != m_mediaPlaybackSession)
	{
		boolean canSeek = 0;
		IFR(m_mediaPlaybackSession->get_CanSeek(&canSeek));

		if (!canSeek)
		{
			IFR(MF_E_INVALIDREQUEST);
		}
		else
		{
			ABI::Windows::Foundation::TimeSpan positionTS;
			positionTS.Duration = position;
			IFR(m_mediaPlaybackSession->put_Position(positionTS));
		}
	}
	return S_OK;
}

HRESULT CMediaPlayerPlayback::GetPlaybackRate(DOUBLE* rate)
{
	Log(Log_Level_Info, L"CMediaPlayerPlayback::GetPlaybackRate()");

	if (nullptr != m_mediaPlaybackSession)
	{
		m_mediaPlaybackSession->get_PlaybackRate(rate);
	}
	return S_OK;
}

HRESULT CMediaPlayerPlayback::SetPlaybackRate(DOUBLE rate) 
{
	Log(Log_Level_Info, L"CMediaPlayerPlayback::SetPlaybackRate()");
	if (nullptr != m_mediaPlaybackSession)
	{
		m_mediaPlaybackSession->put_PlaybackRate(rate);
	}

	return S_OK;
}


_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::CreateMediaPlayer()
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::CreateMediaPlayer()");

    // create media player
    ComPtr<IMediaPlayer> spMediaPlayer;
    IFR(ActivateInstance(Wrappers::HStringReference(RuntimeClass_Windows_Media_Playback_MediaPlayer).Get(), &spMediaPlayer));

    // setup callbacks
    EventRegistrationToken openedEventToken;
    auto mediaOpened = Microsoft::WRL::Callback<IMediaPlayerEventHandler>(this, &CMediaPlayerPlayback::OnOpened);
    IFR(spMediaPlayer->add_MediaOpened(mediaOpened.Get(), &openedEventToken));

    EventRegistrationToken endedEventToken;
    auto mediaEnded = Microsoft::WRL::Callback<IMediaPlayerEventHandler>(this, &CMediaPlayerPlayback::OnEnded);
    IFR(spMediaPlayer->add_MediaEnded(mediaEnded.Get(), &endedEventToken));

    EventRegistrationToken failedEventToken;
    auto mediaFailed = Microsoft::WRL::Callback<IFailedEventHandler>(this, &CMediaPlayerPlayback::OnFailed);
    IFR(spMediaPlayer->add_MediaFailed(mediaFailed.Get(), &failedEventToken));

    // frameserver mode is on the IMediaPlayer5 interface
    ComPtr<IMediaPlayer5> spMediaPlayer5;
    IFR(spMediaPlayer.As(&spMediaPlayer5));

    // set frameserver mode
    IFR(spMediaPlayer5->put_IsVideoFrameServerEnabled(true));

    // register for frame available callback
    EventRegistrationToken videoFrameAvailableToken;
    auto videoFrameAvailableCallback = Microsoft::WRL::Callback<IMediaPlayerEventHandler>(this, &CMediaPlayerPlayback::OnVideoFrameAvailable);
    IFR(spMediaPlayer5->add_VideoFrameAvailable(videoFrameAvailableCallback.Get(), &videoFrameAvailableToken));

    // store the player and token
    m_mediaPlayer.Attach(spMediaPlayer.Detach());
    m_openedEventToken = openedEventToken;
    m_endedEventToken = endedEventToken;
    m_failedEventToken = failedEventToken;
    m_videoFrameAvailableToken = videoFrameAvailableToken;

    IFR(AddStateChanged());

    return S_OK;
}

_Use_decl_annotations_
void CMediaPlayerPlayback::ReleaseMediaPlayer()
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::ReleaseMediaPlayer()");

    RemoveStateChanged();

    if (nullptr != m_mediaPlayer)
    {
        ComPtr<IMediaPlayer5> spMediaPlayer5;
        if (SUCCEEDED(m_mediaPlayer.As(&spMediaPlayer5)))
        {
            LOG_RESULT(spMediaPlayer5->remove_VideoFrameAvailable(m_videoFrameAvailableToken));

            spMediaPlayer5.Reset();
            spMediaPlayer5 = nullptr;
        }

        LOG_RESULT(m_mediaPlayer->remove_MediaOpened(m_openedEventToken));
        LOG_RESULT(m_mediaPlayer->remove_MediaEnded(m_endedEventToken));
        LOG_RESULT(m_mediaPlayer->remove_MediaFailed(m_failedEventToken));

        // stop playback
        LOG_RESULT(Stop());

        m_mediaPlayer.Reset();
        m_mediaPlayer = nullptr;
    }
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::CreateTextures()
{
    if (nullptr != m_primaryTexture || nullptr != m_primaryTextureSRV)
        ReleaseTextures();

    // create staging texture on unity device
    ComPtr<ID3D11Texture2D> spTexture;
    IFR(m_d3dDevice->CreateTexture2D(&m_textureDesc, nullptr, &spTexture));

    auto srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(spTexture.Get(), D3D11_SRV_DIMENSION_TEXTURE2D);
    ComPtr<ID3D11ShaderResourceView> spSRV;
    IFR(m_d3dDevice->CreateShaderResourceView(spTexture.Get(), &srvDesc, &spSRV));

    // create shared texture from the unity texture
    ComPtr<IDXGIResource1> spDXGIResource;
    IFR(spTexture.As(&spDXGIResource));

    HANDLE sharedHandle = INVALID_HANDLE_VALUE;
    ComPtr<ID3D11Texture2D> spMediaTexture;
    ComPtr<IDirect3DSurface> spMediaSurface;
    HRESULT hr = spDXGIResource->CreateSharedHandle(
        nullptr,
        DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
        L"SharedTextureHandle",
        &sharedHandle);
    if (SUCCEEDED(hr))
    {
        ComPtr<ID3D11Device1> spMediaDevice;
        hr = m_mediaDevice.As(&spMediaDevice);
        if (SUCCEEDED(hr))
        {
            hr = spMediaDevice->OpenSharedResource1(sharedHandle, IID_PPV_ARGS(&spMediaTexture));
            if (SUCCEEDED(hr))
            {
                hr = GetSurfaceFromTexture(spMediaTexture.Get(), &spMediaSurface);
            }
        }
    }

    // if anything failed, clean up and return
    if (FAILED(hr))
    {
        if (sharedHandle != INVALID_HANDLE_VALUE)
            CloseHandle(sharedHandle);

        IFR(hr);
    }

    m_primaryTexture.Attach(spTexture.Detach());
    m_primaryTextureSRV.Attach(spSRV.Detach());

    m_primarySharedHandle = sharedHandle;
    m_primaryMediaTexture.Attach(spMediaTexture.Detach());
    m_primaryMediaSurface.Attach(spMediaSurface.Detach());

    return hr;
}

_Use_decl_annotations_
void CMediaPlayerPlayback::ReleaseTextures()
{
    Log(Log_Level_Info, L"CMediaPlayerPlayback::ReleaseTextures()");

    // primary texture
    if (m_primarySharedHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_primarySharedHandle);
        m_primarySharedHandle = INVALID_HANDLE_VALUE;
    }

    m_primaryMediaSurface.Reset();
    m_primaryMediaSurface = nullptr;

    m_primaryMediaTexture.Reset();
    m_primaryMediaTexture = nullptr;

    m_primaryTextureSRV.Reset();
    m_primaryTextureSRV = nullptr;

    m_primaryTexture.Reset();
    m_primaryTexture = nullptr;

    ZeroMemory(&m_textureDesc, sizeof(m_textureDesc));
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::AddStateChanged()
{
    ComPtr<IMediaPlayer3> spMediaPlayer3;
    IFR(m_mediaPlayer.As(&spMediaPlayer3));

    ComPtr<IMediaPlaybackSession> spSession;
    IFR(spMediaPlayer3->get_PlaybackSession(&spSession));
	
	EventRegistrationToken stateChangedToken;
    auto stateChanged = Microsoft::WRL::Callback<IMediaPlaybackSessionEventHandler>(this, &CMediaPlayerPlayback::OnStateChanged);
    IFR(spSession->add_PlaybackStateChanged(stateChanged.Get(), &stateChangedToken));
	IFR(spSession->add_PositionChanged(stateChanged.Get(), &stateChangedToken));
    m_stateChangedEventToken = stateChangedToken;
	
	m_mediaPlaybackSession.Attach(spSession.Detach());

    return S_OK;
}

_Use_decl_annotations_
void CMediaPlayerPlayback::RemoveStateChanged()
{
    // remove playback session callbacks
    if (nullptr != m_mediaPlaybackSession)
    {
		LOG_RESULT(m_mediaPlaybackSession->remove_PlaybackStateChanged(m_stateChangedEventToken));
		LOG_RESULT(m_mediaPlaybackSession->remove_PositionChanged(m_positionChangedEventToken));

        m_mediaPlaybackSession.Reset();
        m_mediaPlaybackSession = nullptr;
    }
}

_Use_decl_annotations_
void CMediaPlayerPlayback::ReleaseResources()
{
    m_fnStateCallback = nullptr;

    // release dx devices
    m_mediaDevice.Reset();
    m_mediaDevice = nullptr;

    m_d3dDevice.Reset();
    m_d3dDevice = nullptr;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::OnVideoFrameAvailable(IMediaPlayer* sender, IInspectable* arg)
{
    ComPtr<IMediaPlayer> spMediaPlayer(sender);

    ComPtr<IMediaPlayer5> spMediaPlayer5;
    IFR(spMediaPlayer.As(&spMediaPlayer5));

    if (nullptr != m_primaryMediaSurface)
    {
        IFR(spMediaPlayer5->CopyFrameToVideoSurface(m_primaryMediaSurface.Get()));
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::OnOpened(IMediaPlayer* sender, IInspectable* args)
{
    ComPtr<IMediaPlayer> spMediaPlayer(sender);

    ComPtr<IMediaPlayer3> spMediaPlayer3;
    IFR(spMediaPlayer.As(&spMediaPlayer3));

    ComPtr<IMediaPlaybackSession> spSession;
    IFR(spMediaPlayer3->get_PlaybackSession(&spSession));

    // width & height of video
    UINT32 width = 0;
    IFR(spSession->get_NaturalVideoWidth(&width));

    UINT32 height = 0;
    IFR(spSession->get_NaturalVideoHeight(&height));

    boolean canSeek = false;
    IFR(spSession->get_CanSeek(&canSeek));

    ABI::Windows::Foundation::TimeSpan duration;
    IFR(spSession->get_NaturalDuration(&duration));

    PLAYBACK_STATE playbackState;
    ZeroMemory(&playbackState, sizeof(playbackState));
    playbackState.type = StateType::StateType_Opened;
    playbackState.value.description.width = width;
    playbackState.value.description.height = height;
    playbackState.value.description.canSeek = canSeek;
    playbackState.value.description.duration = duration.Duration;

    if (m_fnStateCallback != nullptr)
        m_fnStateCallback(playbackState);

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::OnEnded(IMediaPlayer* sender, IInspectable* args)
{
    PLAYBACK_STATE playbackState;
    ZeroMemory(&playbackState, sizeof(playbackState));
    playbackState.type = StateType::StateType_StateChanged;
    playbackState.value.state = PlaybackState::PlaybackState_Ended;

    if (m_fnStateCallback != nullptr)
        m_fnStateCallback(playbackState);

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::OnFailed(IMediaPlayer* sender, IMediaPlayerFailedEventArgs* args)
{
    HRESULT hr = S_OK;
    IFR(args->get_ExtendedErrorCode(&hr));

    SafeString errorMessage;
    IFR(args->get_ErrorMessage(errorMessage.GetAddressOf()));

    LOG_RESULT_MSG(hr, errorMessage.c_str());

    PLAYBACK_STATE playbackState;
    ZeroMemory(&playbackState, sizeof(playbackState));
    playbackState.type = StateType::StateType_Failed;
    playbackState.value.hresult = hr;

    if (m_fnStateCallback != nullptr)
        m_fnStateCallback(playbackState);

    return S_OK;
}

_Use_decl_annotations_
HRESULT CMediaPlayerPlayback::OnStateChanged(IMediaPlaybackSession* sender, IInspectable* args)
{
	ABI::Windows::Foundation::TimeSpan pos;
	IFR(sender->get_Position(&pos));

    MediaPlaybackState state;
    IFR(sender->get_PlaybackState(&state));

    PLAYBACK_STATE playbackState;
    ZeroMemory(&playbackState, sizeof(playbackState));
    playbackState.type = StateType::StateType_StateChanged;
    playbackState.value.state = static_cast<PlaybackState>(state);
	playbackState.value.position = pos;

     if (m_fnStateCallback != nullptr)
        m_fnStateCallback(playbackState);

    return S_OK;
}
