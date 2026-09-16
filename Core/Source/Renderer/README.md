# Current NVRHI based Renderer implementation

## Introduction

This document tries to give a quick overview on how Mupfel currently implements rendering. On the one hand, this helps anyone that is using Mupfel or wanting to know how it works, on the other hand, this helps me in structuring my thoughs and gathering what i have done so far w. r. t. the rendering side of the engine.

## Dependencies

Currently, Mupfel has **2** main dependencies regarding the rendering:

1. **NVRHI** (https://github.com/NVIDIA-RTX/NVRHI/tree/main ) MIT

2. **vk-bootstrap** (https://github.com/charles-lunarg/vk-bootstrap/tree/main ) MIT

### vk-bootstrap

Let's talk about vk-bootstrap first. As Mupfel is currently (only) supporting vulkan, I did not want to re-implement the process of creating all the vulkan related data structures needed for NVRHI (I already did that once in PingRHI). Thus, I am using vk-bootstrap to help me with that.

### NVRHI

While I previously used my own abstraction layer over Vulkan (see https://github.com/Anstapen/PingRHI ), I've now decided to switch that out to NVRHI, mainly for 2 reasons:

* to profit from the additional abstraction which should simplify the overall code and reduce SLOCs

* to profit from the additional features NVRHI provides, namely "resource state tracking" and "automatic barrier placement"

* to maybe also implement D3D12 in the future

## Initialization

The renderer initialization is currently implemented as "deferred initialization": its constructor is default-created by the compiler and all initialization is done in Renderer::Init(). This unfortunately enables the invariant of an uninitialized renderer, but i am not sure if i could solve that at construction time. Luckily, all public functions, except Init(), return void, so the invariance does not propagate to other parts of the engine.

For the initialization, the renderer needs the window, which currently is a GLFW based window structure under the hood.

The most important part of the renderer initialization is checking whether or not required and optional rendering features are supported by the underlying hardware. Mupfel aims to keep the list of required features as small as possible and, for all optional features, tries to be as transparent as possible about their impact.

### Current Vulkan extensions and layers

#### Validation Layers

Vulkan validation layers are only enabled in Debug builds, meaning as long as the symbol NDEBUG is **not defined**. In that case, Mupfel enables all layers found by vk-bootstrap.

#### Optional and Required Instance Extensions

Currently, there are no mandatory or optinal instance extensions.

#### Optional and Required Device Extensions

##### Required

* shaderDrawParameters (Vulkan 1.1)

* timelineSemaphore (Vulkan 1.2)

* descriptorBindingPartiallyBound (Vulkan 1.2)

* shaderSampledImageArrayNonUniformIndexing (Vulkan 1.2)

* synchronization2 (Vulkan 1.3)

* dynamicRendering (Vulkan 1.3)

##### Optional

Currently, there are no optional device extensions.

### 

## Rendering Loop

There are a couple of important values that are used to ensure a correct and responsive behavior of the Renderer. The main Renderer is really only responsible for synchronization of the different parts. This is done as follows:

### frames_in_flight -  CPU-GPU synchronization

In this Renderer, we essentially want to draw the current state of the world. That world is simulated by other parts of the engine, which execute on the CPU. While a lot of the things that are simulated are exclusive to the CPU, there is certain data (like entity position, rotation, texture etc.) that also is of interest to the GPU (to draw the world). That causes some problems, because both the GPU and CPU would like to access the world data, with one (the CPU) most likely changing it every frame. But what happens if the CPU is changing the world while the GPU tries to draw it? Wo somehow need to ensure the data is accessed exclusivly by both systems. The naive approach would maybe be the following:

> Simulate the world using the CPU, then notify the GPU to draw it. After notifying, wait on the CPU until the GPU has finished drawing.

But now, the GPU can only draw the world once the CPU has finished simulating it **and** the CPU can only start simulating further after the GPU has finished drawing it.

The common approach for that problem is **Double Buffering**.

Instead of one world on which both sides operate on, we **duplicate** it and assign one to the CPU to simulate and the other one to the GPU to render it. This is what the frames_in_flight variable stands for. Its the number of duplicate buffers that exist to hold data used for rendering. Currently, this value is set to **2**. That changes the approach above to:

> Simulate the world on the CPU. Wait for the GPU to finish the current buffer. Update the render data of the current buffer. Instruct the GPU to render the current buffer. Set the previous buffer to the current buffer.

This approach solves the CPU-GPU synchronization problem. Nice!

### image count - GPU-screen synchonization

But the above synchronization only ensures that data shared by the GPU and CPU (the buffers for the render data, the draw commands etc.) are not used concurrently. But what about the image that is produced by the GPU and shown on the Screen? The GPU basically does a bunch of math to figure out the color values of a buffer in memory, which is then read by some "presentation engine" that forwards the pixels to the screen. If the GPU writes values in this buffer while it is read to screen, the user can see weird atrifacts. So we do not only need to synchronize data shared between the GPU and CPU, but also data shared between the GPU and the presentation engine (i.e. the "images").

An image is used by the GPU and the presentation engine and basically has two states:

* the presentation of the image by the presentation engine is completed, it may be used by the GPU.

* the drawing to the image by the GPU is completed, it may be presented by the presentation engine.

To notify each other, the two systems (presentation engine and GPU) can use semaphores.

### Overview of the Renderer loop

As stated above, the main renderer (Mupfel::Renderer) is responsible for synchronization and selection of the correct frame to render to. The subrenderers only issue draw commands and bind their resources.
