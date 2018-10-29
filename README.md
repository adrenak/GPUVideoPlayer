# GPUVideoPlayer
Alternative to Unity's `VideoPlayer` component. RUn HEVC/H265 videos with GPU decoding, lower loading times and better performance.

# API/Usage
The `GPUVideoPlayer` class derives from `MonoBehaviour` and needs to be on the scene. `GPUVideoPlayer` component also provides some primitive auto play features.  
  
### Methods:
- `Load(string) : void`  
Where the passed parameter is the URL or path of the video  
- `Play() : bool`  
Which plays (or resumes) the video playback and returns a boolean based on whether the command was successful  
- `Pause() : bool`  
Pauses the video and returns if the command was successful  
- `Stop() : bool`  
Stops the video playback and returns if the command was successful  
- `GetPlaybackRate() : double`  
Returns the rate at which the video is being played  
- `GetDuration() : long`  
Returns the length of the video in 1/10^7 seconds  
- `SeekByRatio(float ratio) : bool`  
Sets the position of the video player at `ratio` completion stage and returns if the attempt was successful  
- `SeekByTime(long position) : bool`  
Sets the position of the video player at `position` time. `position` is in `1/10^7` second units

### C# Properties:  
- `MediaTexture`  
Returns the `Texture2D` object that is updated by the plugin with video frames  
- `MediaDescription`  
Returns some information of the video being played. These include the video width, height, duration and whether it can be seeked on.

### UnityEvents:
- `OnLoaded`  
When the player is done loading a video successfully
- `OnFailed`  
When he player fails to load a video  
- `OnPlay`  
When the player starts the playback of the video  
- `OnPaused`  
When the playback is paused  
- `OnStopped`  
When the playback is stopped  
- `OnEnded`  
Whent he video playback is completed  

# Performance
- `GPUVideoPlayer` was tested with an 800mb `8192x4096` 30FPS H265 MP4 video file. Loading took 195 ms. Video playback was at 30FPS with Unity's framerate at 60.

# Getting Started  
- A simple video player app with load, pause, stop, 5sec FWD, 5sec BWD is available inside the Demo folder

# Notes
- Currently runs only on Microsoft Windows. Tested on 64 bit OS.

# Contact
[@github](https://www.github.com/adrenak)  
[@www](http://www.vatsalambastha.com)