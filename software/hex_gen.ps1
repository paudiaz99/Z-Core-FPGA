# Compile and Generate HEX for Z-Core
# Assumes riscv64-unknown-elf-gcc and Python 3 are in PATH

param(
    [string]$Target = "led_test"
)

$CC = "riscv64-unknown-elf-gcc"
$CFLAGS = "-march=rv32i -mabi=ilp32 -O2 -nostartfiles -T linker.ld"

# Check if linker script exists, if not create a simple one
if (-not (Test-Path "linker.ld")) {
    Write-Host "Creating simple linker script..."
    $linkerScript = @"
OUTPUT_ARCH( "riscv" )
ENTRY( main )
SECTIONS
{
  . = 0x00000000;
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data : { *(.data*) }
  .bss : { *(.bss*) }
}
"@
    Set-Content -Path "linker.ld" -Value $linkerScript
}

Write-Host "Compiling $Target.c..."
$compileArgs = $CFLAGS.Split(' ') + @("-o", "$Target.elf", "$Target.c")
& $CC @compileArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host "Generating HEX file using elf2hex.py..."
    python elf2hex.py "$Target.elf" "$Target.hex"
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Success! Created $Target.hex"
    } else {
        Write-Host "HEX generation failed"
    }
} else {
    Write-Host "Compilation failed"
}
