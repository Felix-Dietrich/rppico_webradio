# rppico_webradio<br>Felix Dietrich 2024
# Internet-Based Radio

## To-Dos<br>
**"Make it work, make it better, make it faster" where we are?**<br>
It's working; we've divided the code more clearly. We're attempting to make signal processing even faster and incorporate some features, such as the USB console for internal monitoring or displaying the Wi-Fi signal strength on the webpage."


- [X] Create todo.md<br>
Solved 3.2.2024 Manfred<br>

### Improvemnets
Memory management
- [ ] Analyze task stack depth (high water level)<br>
- [ ] Ensuring memory operations are secure<br>
- [ ] Web server<br>
- [ ] Flash space (for additional SSIDs and passwords)<br>

Code
- [ ] Thread-safe debug output<br>
  Function ``dbg_print(char *text)`` which pushes the textpointer into a queue.<br>
  Utilize local buffers and ``snprintf`` to print formatet text into the buffer.<br>
  This is fast, thread-safe, and scalable.<br>
  
- [ ]Function ``set_debug_level(int level)``<br>
  level = 0 -> No output<br>
  level = 1 -> Print messages<br>
  
	Keep it simple; when there is too much functionality within the function `set_debug_level(int level)`,<br>
  careful consideration must be given to whether the function needs to be protected by a mutex to ensure thread safety.<br>


### Features
- [ ] USB Console<br>
**Commands:**<br>
  - [ ] update firmware<br>
  - [ ] get stackinfo<br>
  - [ ] get heapinfo<br>
  - [ ] get wifiinfo<br>
	
- [ ] Battery status updates automatically
- [ ] Start the station via web interface
- [ ] Adjust volume through web interface
- [ ] Noise for EQ tuning
- [ ] EQ with filter bank for better resolution at low frequencies
- [ ] Display the current station
- [ ] Windowing in EQ for better frequency separation
- [ ] Display Wi-Fi signal strength
- [ ] Save the last SSIDs and passwords






EOF :-)





