rem Echo do not run this file directly.
rem type build -cz to create a package

rem %1 %2 - version
rem %3 - obj path

if "x%3" == "x" goto end

rem go to root of tree
cd ..\..

set DROP=..\openparport-%1.%2

echo \CVS\ 			> %TEMP%\nojunk.txt
echo \objfre_w2K_x86\		>> %TEMP%\nojunk.txt
echo \objchk_w2K_x86\		>> %TEMP%\nojunk.txt
echo \buildfre_w2K_x86.		>> %TEMP%\nojunk.txt
echo \buildCHK_w2K_x86.		>> %TEMP%\nojunk.txt

mkdir %DROP%
del /q /s %DROP%\*

mkdir %DROP%\doc
xcopy .\doc\*.rtf %DROP%\doc

mkdir %DROP%\bin
xcopy .\src\dlportio\%3\*.dll %DROP%\bin
xcopy .\src\inf\* %DROP%\bin
xcopy .\src\driver\%3\*.sys %DROP%\bin


mkdir %DROP%\include
xcopy /s /exclude:%TEMP%\nojunk.txt .\src\include\* %DROP%\include

mkdir %DROP%\lib
xcopy .\src\dlportio\%3\*.lib %DROP%\lib
xcopy .\src\ppdev\%3\*.lib %DROP%\lib

mkdir %DROP%\src
xcopy /s /exclude:%TEMP%\nojunk.txt .\src\* %DROP%\src

:end
echo end
