@echo off
@rem DOS Batch file to invoke the StarSpan GUI
@rem $Id$
SET STARSPANGUIDIR="$INSTALL_PATH"
java -jar "%STARSPANGUIDIR%\bin\guie.jar" "%STARSPANGUIDIR%\bin\starspan.guie"
@echo on

