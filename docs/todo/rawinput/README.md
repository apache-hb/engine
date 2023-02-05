# raw input support

* enabling raw input by means of `RegisterRawInputDevices` disables standard window messages, this prevents dear imgui from working.
    * we could write our own dear imgui rawinput support, might be possible.
    
* this also prevents `input::Mouse` & `input::Keyboard` from working as expected.
    * this could be remedied by converting WM_INPUT messages into WM_* messages manually using PostMessage

* i am unsure of how to revert that either so it would be a option that requires restarting to disable, which is suboptimal.
* remains here until all of these points are resolved