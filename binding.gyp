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
                'OTHER_CPLUSPLUSFLAGS' : ["-std=c++17"],
                'OTHER_LDFLAGS': [
                    '-framework Foundation',
                    '-framework CoreFoundation',
                    '-framework AudioToolbox'
                ],
            },
            "sources": [ "src/cpp/main.cc", "src/cpp/mic_detector/MicDetector.cc", 'src/cpp/darwin/DeviceInterface.cc'],
        }], # OS==mac
      ],
      'cflags!': ['-ansi', '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions', "-std=c++17" ],
      'cflags': ['-g', '-exceptions'],
      'cflags_cc': ['-g', '-exceptions', "-std=c++17" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      # 'defines': [ 'NAPI_CPP_EXCEPTIONS',  "NAPI_VERSION=<(napi_build_version)" ]
    }
  ]
}
