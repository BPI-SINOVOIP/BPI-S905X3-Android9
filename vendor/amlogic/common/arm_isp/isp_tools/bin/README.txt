/*
 *
 * Android control tool guide
 *
 */

1.code compile
	a.
		cp -r control_tool_android hardware/amlogic/camera

		cd control_tool_android/libevent

		mm
		
		there will output libisptool.a in android out dir,

		then copy libisptool.a to control_tool/act-server/lib

	b.	cd control_tool_android/act-server

		mm

		there will output act-server in android out dir

2.environment
	1)file prepare
		on android
			act-server
			control_tool/act-client/content
			preset file chardev-sw.set
		
		note(act-server,chardev-sw.set,content these three must in one dir)
	
	2)pc prepare
		arm_isp/driver/control_tool_files/IV009-SW-Control.xml

3.connection
	1)on android
		1.connect the network
		2.open android camera app
		3.run ./act-server --preset=chardev-sw.set
		4.if set successfully, it shows in serial port
				Control Tool 3.5.1 of 2018-01-10 15:58:59 +0000 (built: Jun 29 2018 15:00:13)
				(C) COPYRIGHT 2013 - 2018 ARM Limited or its affiliates.
				00:21:10.177   INFO > HTTPServer: please open http://10.18.29.119:8000/ in firefox or chrome
				00:21:15.714   INFO > BusManager: driver chardev has been successfully initialized in sync mode for fw-channel
				00:21:15.714   INFO > CharDeviceDriver: successfully opened device /dev/ac_isp
				00:21:15.714   INFO > CommandManager: access manager configured successfully
				00:21:15.714   INFO > CommandManager: hw access type: stream
				00:21:15.715   INFO > CommandManager: fw access type: stream
	
	2)on pc
		1.after all the set on linux, open chrome/firefox,input ip address
		http://10.18.29.119:8000/
		2.if all the settings are set, the webpage will note you to load xml, then choose IV009-SW-Control.xml
		3.you will see the isp tool window

thus,isp tool environment build complete.

