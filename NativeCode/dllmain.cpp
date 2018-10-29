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
#include "Unity/PlatformBase.h"
#include "MediaPlayerPlayback.h"

using namespace Microsoft::WRL;

static UnityGfxRenderer s_DeviceType = kUnityGfxRenderernullptr;
static IUnityInterfaces* s_UnityInterfaces = nullptr;
static IUnityGraphics* s_Graphics = nullptr;

static ComPtr<IMediaPlayerPlayback> s_spMediaPlayback;
static float g_Time;


STDAPI_(BOOL) DllMain(
    _In_opt_ HINSTANCE hInstance, _In_ DWORD dwReason, _In_opt_ LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    if (DLL_PROCESS_ATTACH == dwReason)
    {
        //  Don't need per-thread callbacks
        DisableThreadLibraryCalls(hInstance);

        Module<InProc>::GetModule().Create();
    }
    else if (DLL_PROCESS_DETACH == dwReason)
    {
        Module<InProc>::GetModule().Terminate();
    }

    return TRUE;
}

STDAPI DllGetActivationFactory(
    _In_ HSTRING activatibleClassId, 
    _COM_Outptr_ IActivationFactory** factory)
{
    auto &module = Module< InProc>::GetModule();
    return module.GetActivationFactory(activatibleClassId, factory);
}

STDAPI DllCanUnloadNow()
{
    const auto &module = Module<InProc>::GetModule();
    return module.GetObjectCount() == 0 ? S_OK : S_FALSE;
}


extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CreateMediaPlayback(_In_ StateChangedCallback fnCallback)
{
    ComPtr<IMediaPlayerPlayback> spPlayerPlayback;
    IFR(CMediaPlayerPlayback::CreateMediaPlayback(s_DeviceType, s_UnityInterfaces, fnCallback, &spPlayerPlayback));

    s_spMediaPlayback.Attach(spPlayerPlayback.Detach());

    return S_OK;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ReleaseMediaPlayback()
{
    if (s_spMediaPlayback != nullptr)
    {
        s_spMediaPlayback.Reset();
        s_spMediaPlayback = nullptr;
    }
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CreatePlaybackTexture(_In_ UINT32 width, _In_ UINT32 height, _COM_Outptr_ void** ppvTexture)
{
    NULL_CHK(ppvTexture);
    NULL_CHK(s_spMediaPlayback);

    return s_spMediaPlayback->CreatePlaybackTexture(width, height, ppvTexture);
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API LoadContent(_In_ LPCWSTR pszContentLocation)
{
    NULL_CHK(s_spMediaPlayback);

    return s_spMediaPlayback->LoadContent(pszContentLocation);
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Play()
{
    NULL_CHK(s_spMediaPlayback);

    return s_spMediaPlayback->Play();
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Pause()
{
    NULL_CHK(s_spMediaPlayback);

    return s_spMediaPlayback->Pause();
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Stop()
{
    NULL_CHK(s_spMediaPlayback);

    return s_spMediaPlayback->Stop();
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetPosition(_Out_ LONGLONG* position)
{
	NULL_CHK(s_spMediaPlayback);

	return s_spMediaPlayback->GetPosition(position);
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetDuration(_Out_ LONGLONG* duration)
{
	NULL_CHK(s_spMediaPlayback);

	return s_spMediaPlayback->GetDuration(duration);
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetPosition(_In_ LONGLONG position)
{
	NULL_CHK(s_spMediaPlayback);

	return s_spMediaPlayback->SetPosition(position);
}

extern "C" HRESULT UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetPlaybackRate(_Out_ DOUBLE* rate)
{
	NULL_CHK(s_spMediaPlayback);

	return s_spMediaPlayback->GetPlaybackRate(rate);
}

// --------------------------------------------------------------------------
// UnitySetInterfaces

// GraphicsDeviceEvent
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    // Create graphics API implementation upon initialization
    if (eventType == kUnityGfxDeviceEventInitialize)
    {
        s_DeviceType = s_Graphics->GetRenderer();
    }

    // Cleanup graphics API implementation upon shutdown
    if (eventType == kUnityGfxDeviceEventShutdown)
    {
        s_DeviceType = kUnityGfxRenderernullptr;
    }
}

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    s_UnityInterfaces = unityInterfaces;
    s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
    s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}


// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTimeFromUnity(float t)
{ 
    g_Time = t;
}

// --------------------------------------------------------------------------
// OnRenderEvent
// This will be called for GL.IssuePluginEvent script calls; eventID will
// be the integer passed to IssuePluginEvent. In this example, we just ignore
// that value.
static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.
extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
    return OnRenderEvent;
}

