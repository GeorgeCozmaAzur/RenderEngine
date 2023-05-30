# Render Engine

A small engine that renders stuff. 
It is written in vulkan for now. It started from https://github.com/SaschaWillems/Vulkan and evolved.

## Setup
```
git clone --recursive https://github.com/GeorgeCozmaAzur/RenderEngine.git
cd RenderEngine
mkdir project
cmake -G "Visual Studio 16 2019" project
```

## Getting the asset pack
The data is not uploaded here in order to keep the storage low. 

### Manual download

Download the asset pack from [here](https://drive.google.com/file/d/1n3LUgbks31gyGP_Spv43xJXuw8tGFrUO/view) and extract it in the ```data``` directory.

### Compile shaders

Call ```compilesAllShaders.py``` to compile all shaders into spirv format. You need to install the vulkan sdk for this.

To compile for an individual folder call ```data/shaders/compileshaders.py -shaderfoldername-```


## The projects

### emptyproject
The basic environment that contains a camera and basic ui.

### simplemodel
Loads a model with all the necessary render elements to render it on the screen.

### deferred
Deferred lighting technique that iterates through all lights in the fragment shader to calculate light values.

### deferredlights
Deferred lighting technique that renders a sphere for all lights in the scene to calculate the ligthing on.

### normalparallax
Normal mapping and parallax mapping in one shader.

### pbr pbribl pbribltextured
PBR techniques.

### reflections
Basic reflections.

### shadowmapping
Basic shadowmapping.

### simpleposteffect
A simple post effect on a scene.

### volumetriclighting
A technique that approximates the light scatter in an environment with dense particles(dust fog etc.).

### wind
Simulates wind on the trunk, branches and leaves by converting to sphere coordinates. Leaves have random wiggling.