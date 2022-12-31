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