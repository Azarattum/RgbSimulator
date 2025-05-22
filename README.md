# QMK RGB Simulator  
Preview QMK RGB matrix animations in real-time without reflashing your keyboard!

![Screenshot 2025-05-22 at 20 31 57](https://github.com/user-attachments/assets/0259cd4d-4c9e-4cae-ae5d-19e54e1cf57c)

## ✨ Features  
- **Live Preview**: Visualize QMK RGB animations instantly.  
- **Reactive Testing**: Click LEDs with your mouse to simulate keypresses for reactive effects.  
- **Adjustable Parameters**: Tweak hue, saturation, value, and speed using keyboard shortcuts (like on a real QMK keyboard).  

## 🛠️ Requirements  
- **macOS/Linux**: SDL2 library (install via Homebrew: `brew install sdl2`)  
- **Windows**: SDL2 development libraries (use MSYS2 or vcpkg)  
- A C compiler (clang/gcc)  

> **Note**: Update the SDL2 include/library paths in the `Makefile` if needed.  

## 🚀 Usage  
### Run an Animation  
```bash  
make run <animation_name>  
# Example:  
make run splash           # Runs animations/splash_anim.h  
make run custom/simplex_noise  # Runs animations/custom/simplex_noise_anim.h  
```  

## ⌨️ Controls  
| Key         | Action                                 |
| ----------- | -------------------------------------- |
| `W` / `S`   | Increase/Decrease Value                |
| `E` / `D`   | Increase/Decrease Hue                  |
| `R` / `F`   | Increase/Decrease Saturation           |
| `T` / `G`   | Increase/Decrease Speed                |
| Mouse Click | Simulate a keypress at the clicked LED |

## 🔧 Troubleshooting  
- **SDL2 Errors**: Ensure the SDL2 paths in the `Makefile` match your system.  
- **Animation Not Found**: Verify the animation name matches the `*_anim.h` filename and `ENABLE_RGB_MATRIX_*` define.  
- **Layout Mismatch**: LED positions are hardcoded in `helpers/layout.h`. Modify for your keyboard’s physical layout.  
