# Texture Painting & Model Import

## Texture painting quick guide (Blender)


1. Combine everything into one model.
2. Apply all transforms and set origin.
2. Tab into Edit Mode, hit `U`, then click **Smart UV Project** with an **Island Margin of 0.050**.
3. Open the **Texture Paint** tab; this automatically opens the Image Editor tab.
4. Add a new texture (**Texture Slots** at the top): **Base Color**, **2048x2048**, turn **off Alpha**; name it something related to the object.
5. Split open a 3rd window for the **Shader Editor**; delete the **Principled BSDF** and connect the color's **Color** output to the **Material Output → Surface**.



6. **Fill Brush | Blend: Mix | Strength 1.000**
- Add a gradient — light at top, dark at bottom
7. **Paint Hard Brush | Blend: Mix | Strength: 0.5**; - Add black shading to the dark areas of the object
8. Using the same brush, add any dark (black) details or extra edges to give the object form.
9. Change brush to **Blend: Screen** and set it to **white** (leave at 0.5 strength); highlight edges
   where light hits:
   - Extremities: lighter
   - Crevices: darker
10. Go back to the **Fill** brush, **Blend: Multiply**, **Black**, **Strength: 0.15**; tap a few times to
    darken the object.



11. Switch back to **Paint Hard**, **Blend: Screen**, **0.35 strength**, **grey** this time — and go back
    over the highlights.
12. Switch back to **Multiply**, **dark gray**, and add any details (cracks, etc.).
13. Switch back to **Screen**, **light gray**, and add highlights to the new details.



## How to import the model (Unreal)

1. Drag the FBX into the Content Drawer.
2. Delete the material instance it comes with.
3. Rename your object to **`SM_object`**.
4. Import your texture, rename to **`T_object_BC`**.
5. Create a new instance of **`M_Furniture`**, assign the new texture to it.
6. Rename to **`MI_object`**.
7. Override the global texture parameter in `MI_object` with **`T_object_BC`**.
8. Open your object and change its material to the **`MI_object`** material.
