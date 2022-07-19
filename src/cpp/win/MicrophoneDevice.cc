#include "MicrophoneDevice.h"

#include <atlbase.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <windows.h>

#include <memory>
#include <vector>

namespace Gloo::Internal::MicDetector::Windows {

HRESULT MicrophoneDevice::winStartTrackingDeviceImpl() {
  CComPtr<IAudioSessionEnumerator> sessionList;
  RETURN_IF_FAILED(_sessionManager->GetSessionEnumerator(&sessionList));
  int count;
  RETURN_IF_FAILED(sessionList->GetCount(&count));
  for (int i = 0; i < count; ++i) {
    CComPtr<IAudioSessionControl> controller;
    RETURN_IF_FAILED(sessionList->GetSession(i, &controller));
    RETURN_IF_FAILED(OnSessionCreated(controller));
  }

  RETURN_IF_FAILED(_sessionManager->RegisterSessionNotification(this));
  return S_OK;
}

HRESULT MicrophoneDevice::winStopTrackingDeviceImpl() {
  RETURN_IF_FAILED(_sessionManager->UnregisterSessionNotification(this));
  for (auto &controller : _sessionControllers) {
    RETURN_IF_FAILED(controller->UnregisterAudioSessionNotification(this));
  }
  _sessionControllers.clear();
  return S_OK;
}

// Takes ownership of device.
HRESULT MicrophoneDevice::Make(const AudioDeviceId &deviceId,
                               CComPtr<IMMDevice> &device,
                               std::shared_ptr<IMicrophoneDevice> &output,
                               IDeviceManager *manager) {
  CComPtr<IAudioSessionManager2> session_manager;
  RETURN_IF_FAILED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                                    nullptr, (void **)(&session_manager)));
  output.reset(new MicrophoneDevice(deviceId, manager, session_manager));
  return S_OK;
}

HRESULT MicrophoneDevice::OnSessionCreated(IAudioSessionControl *session) {
  CComPtr<IAudioSessionControl2> audioSessionControl;
  IF_SUCCEEDED(session->QueryInterface(&audioSessionControl)) {
    if (audioSessionControl->IsSystemSoundsSession() == S_FALSE) {
      if (spdlog::get_level() >= spdlog::level::debug) {
        CComHeapPtr<WCHAR> name;
        LOG_IF_FAILED(audioSessionControl->GetDisplayName(&name));
        spdlog::debug("New session (Device:{}) {} {}", _deviceId, _sessionControllers.size(), to_utf8(name.m_pData));
      }

      AudioSessionState state = AudioSessionState::AudioSessionStateExpired;
      LOG_IF_FAILED(audioSessionControl->GetState(&state));
      winUpdateState(state, true);

      IF_SUCCEEDED(
          audioSessionControl->RegisterAudioSessionNotification(this)) {
      _sessionControllers.push_back(audioSessionControl);
          }
    }
  }
  return S_OK;
}

void MicrophoneDevice::winUpdateState(AudioSessionState state, bool initialCall) {
  if (state != AudioSessionState::AudioSessionStateActive && state != AudioSessionState::AudioSessionStateInactive) return;
  
  bool active = state == AudioSessionState::AudioSessionStateActive;
  // Don't increment the device if a session turns on.
  int increment = active ? 1 : (initialCall ? 0 : -1);

  const int prev = _micActivity.fetch_add(increment, std::memory_order_relaxed);
  const int next = prev + increment;
  spdlog::debug("Device::({}) {} {}->{}", _deviceId, (state ? "ON" : "OFF"), prev, next);

  if ((prev > 0) != (next > 0)) {
    refreshState(active, initialCall);
  }
}
}  // namespace Gloo::Internal::MicDetector