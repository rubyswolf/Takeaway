@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /std:c++17 /EHsc /utf-8 /O2 /DNDEBUG /Fo"C:\Users\spa0028\Documents\Uni\Second Year Semester 1\Week 7\Takeaway\.codex-build-progress-fix\\" "strategy tester.cpp" "strategy.cpp" /link /OUT:"C:\Users\spa0028\Documents\Uni\Second Year Semester 1\Week 7\Takeaway\.codex-build-progress-fix\strategy_tester_check.exe"
exit /b %errorlevel%
