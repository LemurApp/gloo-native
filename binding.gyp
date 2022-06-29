{
  "targets": [
    {
      "target_name": "mic_detector",
      "cflags_cc!": [ "-std=c++17", "-DNAPI_CPP_EXCEPTIONS", "-fexceptions", "-Werror"],
      'conditions': [
        ['OS == "mac"', {
          "sources": [ "src/cpp/main.cc", "src/cpp/mic_detector/MicDetector.cc", 'src/cpp/darwin/DeviceInterface.cc'],
          "xcode_settings": {
            "OTHER_CFLAGS": [ "-std=c++17", "-DNAPI_CPP_EXCEPTIONS", "-fexceptions", "-Werror"],
          },
          "link_settings": {
            "libraries": [
              "-framework",
              "Foundation",
              "$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework",
              "$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework",
            ]
          }
        }],
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'defines': [ 'NAPI_CPP_EXCEPTIONS' ],
    }
  ]
}
