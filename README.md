# PX4-FastRTPS-PoC-Snapdragon-UDP

## The spirit

This is a proof of concept aims to check the solution of a need to the Qualcomm platform Snapdragon Flight. This need is to add a UDP transport for the bridge between PX4 and Fast RTPS.

For this reason we create here an example of the usage of the new UDP node created for this task over a single message *sensor_combined* including both Micro RTPS agent and Micro RTPS client.

## How to use?

Before to start work we need two things accomplished:

  - **Fast RTPS** cross compiled for the Snapdragon Flight ([see how](https://github.com/ATLFlight/cross_fastrtps)).
  - **eProsima/Firmware** [repository](https://github.com/eProsima/Firmware) on the last version.

After that reaches, we will start with this repository:

  - Copy the client into the Firmware tree in this way:

  ```sh
  $ cp -r /path/to/this/repo/micrortps_client_udp /path/to/eprosima-firmware/src/examples/
  ```
  - Enable the compilation of the example uncommenting on the file cmake/configs/**posix_sdflight_default.cmake** of *eProsima/Firmware* this line:

  ```sh
       # micro-RTPS
       #examples/micrortps_client_udp
  ```
  ```sh
       # micro-RTPS
       examples/micrortps_client_udp
  ```
  - Compile and upload the client:

  ```sh
  $ cd /path/to/eprosima-firmware/
  $ make eagle_default upload
  ```
  - Compile and upload the agent, indicating the Fast RTPS cross compiling install path (make sure you have exported properly of the [Hexagon SDK](https://github.com/ATLFlight/cross_toolchain/blob/master/README.md) environment variables):

  ```sh
  $ cd /path/to/this/repo/micrortps_agent_udp
  $ make CROSS_FASTRTPS_DIR=/path/to/cross_fastrtps/install load
  ```
  - Launch the agent:

  ```sh
  $ adb shell
  linaro$ cd /home/linaro
  linaro$ ./micrortps_agent_udp
  ```

  - Launch the client in a separate shell:

  ```sh
  $ adb shell
  linaro$ cd /home/linaro && ./px4 mainapp.config
  pxh> micrortps_client_udp
  ```

The output of both applications show a part of the message *sensor_combined*, specifically the barometer temperature. First sent and them received again in the client and upside down in the agent. 58 is the id of the message:

  ```sh
  [58]>> 41.599998
                >>[58] 41.599998
  ```
  (extract from client)
