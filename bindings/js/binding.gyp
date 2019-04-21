{
    "targets": [
        {
            "target_name": "pwned_passwords",
            "sources": [
                "binding.cpp", "../../src/services/hash.cpp"
            ],
            "include_dirs": [
                "/usr/local/opt/openssl/include",
                "<!@(node -p \"require('node-addon-api').include\")",
                "../../include",
                "../../lib/xxHash",
            ],
            "dependencies": [
                "<!(node -p \"require('node-addon-api').gyp\")",
            ],
            "libraries": [
                "-L/usr/local/opt/openssl/lib",
            ],
            "cflags!": ["-fno-exceptions"],
            "cflags_cc!": ["-fno-exceptions"],
            "cflags": ["-std=c++11", "-stdlib=libc++"],
            "defines": ["NAPI_CPP_EXCEPTIONS"],
            "conditions":[
                ['OS=="mac"', {
                    "xcode_settings": {
                        "MACOSX_DEPLOYMENT_TARGET": "10.9",
                        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                        'OTHER_CFLAGS': ['-std=c++11', '-stdlib=libc++'],
                        'OTHER_CPLUSPLUSFLAGS': ['-std=c++11', '-stdlib=libc++']
                    }
                }]
            ]

        }
    ]
}
