Please follow the below procedure to test webOS audio framework features on RPI3 device, after flashing the webOS binary on the target.

Bare app is already available in the below path on the target device. That can be used as reference.
/usr/palm/applications/bareapp

To make audio test application available in place of bareapp, below procedure need to be adapted.

1) Go to the TestApp folder of audiod-pro source code.
   Path : audiod-pro/HTML5_test_App/TestApp
2) Replace the index.html of bareapp with the index.html from audiod-pro/HTML5_test_App/TestApp.
3) Copy the webOSjs-0.1.0 folder from audiod-pro/HTML5_test_App/TestApp to /usr/palm/applications/bareapp
   NOTE:webOS.js file is to communicate internally with the webOS platform
4) open the index.html file
    vi /usr/palm/applications/bareapp/index.html
5) According to the user requirement,please provide the video URL in index.html video tag source input as below
   <source src="Please give URL" type="video/mp4">
   Example:<source src="http://10.195.252.40/media_content/videos/tears_of_steel_1080p.mp4" type="video/mp4">
6) Reboot the RPI device and launch bareapp from the launcher.
7) Start playing the video on clicking the play button.

Ready to verify...

Following controls are available to test.

Mute                      Audio will be muted
                          (Same button changes to un-mute)
Un-mute                   Audio will be un-muted
                          (Same button changes to mute)
Volume                    Enter any value from 0-100 value and press OK button
volumeUp                  To increase the volume
volumeDown                To decrease the volume
Connect-headset           Audio will be heard from the headset connected to the
                          3.5 mm jack available on the target RPI.
                          (Same button changes to disconnect-headset)
Disconnect-headset        Audio will be heard from the HDMI monitor connected
                          (Same button changes to connect-headset)
Navigation                Small notification sound will be heard and then audio
                          of the video continues from the application.
