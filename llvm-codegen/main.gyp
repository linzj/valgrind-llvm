{
    'includes': [
        './build/common.gypi',
    ],
    'targets': [
        {
            'target_name': 'main',
            'type': 'executable',
            'sources': [
                'main.cpp'
             ],
            'dependencies': [
                '<(DEPTH)/llvm/llvm.gyp:libllvm',
            ]
        },
    ],
}
