if not exist x64\Debug\ mkdir x64\Debug
if not exist x64\Release\ mkdir x64\Release

for /d %%d in (libs/*) do (
	for %%f in (libs/%%d/*.dll) do (
		if not exist "x64/Debug/%%f" (copy "libs\%%d\%%f" "x64\Debug\%%f")
		if not exist "x64/Release/%%f" (copy "libs\%%d\%%f" "x64\Release\%%f")
	)
)