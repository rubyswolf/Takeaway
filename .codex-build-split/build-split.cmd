@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /EHsc /utf-8 /O2 /GL /arch:AVX2 /DNDEBUG /DVERIFY_ROOT_SLICE_INDEX=0 /DVERIFY_ROOT_SLICE_COUNT=2 /Fo"C:\Users\spa0028\Documents\Uni\Second Year Semester 1\Week 7\Takeaway\.codex-build-split\split1\\" "strategy tester.cpp" "strategy.cpp" /Fe:"test winning strategy AVX2 1.exe" /link /LTCG
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /EHsc /utf-8 /O2 /GL /arch:AVX2 /DNDEBUG /DVERIFY_ROOT_SLICE_INDEX=1 /DVERIFY_ROOT_SLICE_COUNT=2 /Fo"C:\Users\spa0028\Documents\Uni\Second Year Semester 1\Week 7\Takeaway\.codex-build-split\split2\\" "strategy tester.cpp" "strategy.cpp" /Fe:"test winning strategy AVX2 2.exe" /link /LTCG
exit /b %errorlevel%
