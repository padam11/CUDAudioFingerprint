# CUDAudioFingerprint
Redeployment

# How to Run
First run footprint-audio.
```
make
./footprint-audio
```
When prompted for a folder, type '../../music-wav', or another folder of your choice, provided all of the songs are 22050 sample rate hz, and are mono. You also have the choice to utilize CUDA.

Then run identify-audio.
```
make
./identify-audio
```

You will be prompted for a song file. Make sure it is 22050 sample rate hz and is mono. You also have the choice to utilize CUDA.