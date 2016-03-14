# Flutter - real-time video stabilization

Flutter is a real-time video stabilization application built on OpenCV.

## Building

    mkdir out && cd out
    cmake ..
    make

You can now run `out/flutter`

## Examples

Get help:

    flutter --help

Stabilize default webcam input with default parameters:

    flutter

Use parameters more suitable to videos with mostly local
movement (like a webcam that doesn't move much):

    flutter -r 0.9 -n 0.02

Use a moving average filter for camera smoothing with a window
length of 30 frames, zoom in by a factor of 1.2 and output both
the original and filtered versions:

    flutter -a 20 -z 1.2 -x

Stabilize a video file, scale to a width of 640 pixels maintaining
aspect ratio, don't show output, use motion-JPG as the output codec
and save the camera trajectory data to a file as tab-separated values:

    flutter pan.mp4 -o out.avi -s 640x -q -c mjpg -t out.tsv

Flutter currently ignores sound in the input video file.

Stabilize input from an Android device with [IP Webcam](https://play.google.com/store/apps/details?id=com.pas.webcam):

    # Assuming you have v4l2loopback kernel module installed.
    # Replace the IP address with your device's IP.
    scripts/ipwebcam -q 192.168.0.3
    
    # OpenCV identifies available devices with a number,
    # usually "0" represents /dev/video0, "1" /dev/video1 etc.
    
    # In another terminal, assuming the loopback device is /dev/video1
    flutter -d 1

## License

MIT
