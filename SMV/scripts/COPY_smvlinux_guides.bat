@echo off

Rem setup environment variables (defining where repository resides etc) 

set envfile="%userprofile%"\fds_smv_env.bat
IF EXIST %envfile% GOTO endif_envexist
echo ***Fatal error.  The environment setup file %envfile% does not exist. 
echo Create a file named %envfile% and use SMV/scripts/fds_smv_env_template.bat
echo as an example.
echo.
echo Aborting now...
pause>NUL
goto:eof

:endif_envexist

call %envfile%

%svn_drive%

echo.
echo ---downloading guides
echo.
set fromdir=%smokebotrepo%/Manuals
set todir="%userprofile%"\FDS_Guides

Title Copying smokeview guides from smokebot to FDS_Guides

pscp %linux_logon%:%fromdir%/SMV_User_Guide/SMV_User_Guide.pdf                                %todir%\.
pscp %linux_logon%:%fromdir%/SMV_Verification_Guide/SMV_Verification_Guide.pdf                %todir%\.
pscp %linux_logon%:%fromdir%/SMV_Technical_Reference_Guide/SMV_Technical_Reference_Guide.pdf  %todir%\.

pause
