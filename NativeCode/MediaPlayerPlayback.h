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
#pragma once

enum class StateType : UINT16
{
    StateType_None = 0,
    StateType_Opened,
    StateType_StateChanged,
    StateType_Failed,
	StateType_PositionChanged,
};

enum class PlaybackState : UINT16
{
    PlaybackState_None = 0,
    PlaybackState_Opening,
    PlaybackState_Buffering,
    PlaybackState_Playing,
    PlaybackState_Paused,
    PlaybackState_Ended
};

#pragma pack(push, 4)
typedef struct _MEDIA_DESCRIPTION
{
    UINT32 width;
    UINT32 height;
    INT64 duration;
    byte canSeek;
} MEDIA_DESCRIPTION;
#pragma pack(pop)

#pragma pack(push, 4)
typedef struct _PLAYBACK_STATE
{
    StateType type;
    union {
        PlaybackState state;
        HRESULT hresult;
        MEDIA_DESCRIPTION description;
		ABI::Windows::Foundation::TimeSpan position;
    } value;
} PLAYBACK_STATE;
#pragma pack(pop)

extern "C" typedef void(UNITY_INTERFACE_API *StateChangedCallback)(
    _In_ PLAYBACK_STATE args);

extern "C" typedef void(UNITY_INTERFACE_API *PositionChangedCallback)(
	_In_ ABI::Windows::Foundation::TimeSpan args);

typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::Media::Playback::MediaPlayer*, IInspectable*> IMediaPlayerEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::Media::Playback::MediaPlayer*, ABI::Windows::Media::Playback::MediaPlayerFailedEventArgs*> IFailedEventHandler;
typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::Media::Playback::MediaPlaybackSession*, IInspectable*> IMediaPlaybackSessionEventHandler;

DECLARE_INTERFACE_IID_(IMediaPlayerPlayback, IUnknown, "9669c78e-42c4-4178-a1e3-75b03d0f8c9a")
{
    STDMETHOD(CreatePlaybackTexture)(_In_ UINT32 width, _In_ UINT32 height, _COM_Outptr_ void** ppvTexture) PURE;
    STDMETHOD(LoadContent)(_In_ LPCWSTR pszContentLocation) PURE;
    STDMETHOD(Play)() PURE;
    STDMETHOD(Pause)() PURE;
	STDMETHOD(Stop)() PURE;
	STDMETHOD(GetPosition)(_COM_Outptr_ LONGLONG* position) PURE;
	STDMETHOD(GetDuration)(_COM_Outptr_ LONGLONG* duration) PURE;
	STDMETHOD(GetPlaybackRate)(_COM_Outptr_ DOUBLE* rate) PURE;
	STDMETHOD(SetPosition)(_In_ LONGLONG position) PURE;
};

class CMediaPlayerPlayback
    : public Microsoft::WRL::RuntimeClass
    < Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>
    , IMediaPlayerPlayback
    , Microsoft::WRL::FtmBase>
{
public:
    static HRESULT CreateMediaPlayback(
        _In_ UnityGfxRenderer apiType, 
        _In_ IUnityInterfaces* pUnityInterfaces, 
        _In_ StateChangedCallback fnCallback,
        _COM_Outptr_ IMediaPlayerPlayback** ppMediaPlayback);

    CMediaPlayerPlayback();
    ~CMediaPlayerPlayback();

    HRESULT RuntimeClassInitialize(
        _In_ StateChangedCallback fnCallback,
        _In_ ID3D11Device* pDevice);

    // IMediaPlayerPlayback
    IFACEMETHOD(CreatePlaybackTexture)(
        _In_ UINT32 width, 
        _In_ UINT32 height, 
        _COM_Outptr_ void** ppvTexture);
    IFACEMETHOD(LoadContent)(
        _In_ LPCWSTR pszContentLocation);

    IFACEMETHOD(Play)();
    IFACEMETHOD(Pause)();
	IFACEMETHOD(Stop)();
	IFACEMETHOD(GetPosition(_COM_Outptr_ LONGLONG* position));
	IFACEMETHOD(GetDuration(_COM_Outptr_ LONGLONG* duration));
	IFACEMETHOD(GetPlaybackRate(_COM_Outptr_ DOUBLE* rate));
	IFACEMETHOD(SetPosition(_In_ LONGLONG position));

protected:
    // Callbacks - IMediaPlayer2
    HRESULT OnOpened(
        _In_ ABI::Windows::Media::Playback::IMediaPlayer* sender,
        _In_ IInspectable* args);
    HRESULT OnEnded(
        _In_ ABI::Windows::Media::Playback::IMediaPlayer* sender,
        _In_ IInspectable* args);
    HRESULT OnFailed(
        _In_ ABI::Windows::Media::Playback::IMediaPlayer* sender,
        _In_ ABI::Windows::Media::Playback::IMediaPlayerFailedEventArgs* args);

    // Callbacks - IMediaPlayer5 - frameserver
    HRESULT OnVideoFrameAvailable(
        _In_ ABI::Windows::Media::Playback::IMediaPlayer* sender,
        _In_ IInspectable* args);

    // Callbacks - IMediaPlaybackSession
    HRESULT OnStateChanged(
        _In_ ABI::Windows::Media::Playback::IMediaPlaybackSession* sender,
        _In_ IInspectable* args);

	HRESULT OnPositionChanged(
		_In_ ABI::Windows::Media::Playback::IMediaPlaybackSession* sender,
		_In_ IInspectable* args);

private:
    HRESULT CreateMediaPlayer();
    void ReleaseMediaPlayer();

    HRESULT CreateTextures();
    void ReleaseTextures();

    HRESULT AddStateChanged();
    void RemoveStateChanged();

    void ReleaseResources();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11Device> m_mediaDevice;

    StateChangedCallback m_fnStateCallback;

    Microsoft::WRL::ComPtr<ABI::Windows::Media::Playback::IMediaPlayer> m_mediaPlayer;
    EventRegistrationToken m_openedEventToken;
    EventRegistrationToken m_endedEventToken;
    EventRegistrationToken m_failedEventToken;
    EventRegistrationToken m_videoFrameAvailableToken;

    Microsoft::WRL::ComPtr<ABI::Windows::Media::Playback::IMediaPlaybackSession> m_mediaPlaybackSession;
	EventRegistrationToken m_stateChangedEventToken;
	EventRegistrationToken m_positionChangedEventToken;

    CD3D11_TEXTURE2D_DESC m_textureDesc;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_primaryTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_primaryTextureSRV;

    HANDLE m_primarySharedHandle;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_primaryMediaTexture;
    Microsoft::WRL::ComPtr<ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface> m_primaryMediaSurface;
};

