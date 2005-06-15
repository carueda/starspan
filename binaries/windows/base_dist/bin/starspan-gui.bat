@echo off
@rem DOS Batch file to invoke the StarSpan GUI
@rem $Id$
SET STARSPANGUIDIR=$INSTALL_PATH
SET SS_SAVEPATH=%PATH%
PATH=%STARSPANGUIDIR%\bin;%STARSPANGUIDIR%\lib;%PATH%
@echo path = %PATH%
@echo Launching StarSpan GUI...
java -jar "%STARSPANGUIDIR%\bin\guie.jar" "%STARSPANGUIDIR%\bin\starspan.guie"
SET PATH=%SS_SAVEPATH%
@echo on

