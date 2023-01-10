{
  "targets": [
    {
      "target_name": "mic_detector",
      'conditions': [
        [ 'OS=="mac"', {
            'LDFLAGS': [
                '-framework Foundation',
                '-framework CoreFoundation',
                '-framework AudioToolbox'
            ],
            'xcode_settings': {
                'CLANG_CXX_LIBRARY': 'libc++',
                'MACOSX_DEPLOYMENT_TARGET': '10.9',
                'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                'OTHER_CPLUSPLUSFLAGS' : ["-std=c++17", "-DDEBUG"],
                'OTHER_LDFLAGS': [
                    '-framework Foundation',
                    '-framework CoreFoundation',
                    '-framework AudioToolbox',
                    '-framework ApplicationServices'
                ],
            },
            "sources": [ "src/cpp/main.cc", "src/cpp/mic_detector/MicDetector.cc", 'src/cpp/darwin/OSXDeviceManager.cc', 'src/cpp/darwin/OSXHelpers.cc', 'src/cpp/darwin/WindowTracker.cc', "src/cpp/screen_tracker/ScreenTracker.cc"],
        }], # OS==mac
        [ 'OS=="win"', {
            "sources": [ "src/cpp/main.cc", "src/cpp/mic_detector/MicDetector.cc", 'src/cpp/win/DeviceInterface.cc', 'src/cpp/win/MicrophoneDevice.cc'],
            'defines': [
              'NAPI_CPP_EXCEPTIONS',
              '_HAS_EXCEPTIONS=1'
            ],
            'msvs_settings': {
              'VCCLCompilerTool': {
                'AdditionalOptions': [ '-std:c++17', ],
                'FavorSizeOrSpeed': 2,  # Favor size over speed
                'Optimization': 1,  # Optimize for size
                'RuntimeLibrary': 2,  # Multi-threaded runtime dll,
                'ExceptionHandling': 1,  # Yes please
              },
            },
        }]
      ],
      'cflags!': ['-ansi', '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions', "-std=c++17" ],
      'cflags': ['-g', '-exceptions'],
      'cflags_cc': ['-g', '-exceptions', "-std=c++17" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "./src/cpp/third_party/spdlog/include"
      ],
    }
  ]
}
