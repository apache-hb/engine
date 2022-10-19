#ifndef DEVICE
#   define DEVICE(id, name)
#endif

DEVICE(eDesktop, "input.device.desktop")
DEVICE(eGamepad, "input.device.gamepad")

#undef DEVICE

#ifndef KEY
#   define KEY(id, name)
#endif

KEY(keyA, "input.key.a")
KEY(keyB, "input.key.b")
KEY(keyC, "input.key.c")
KEY(keyD, "input.key.d")
KEY(keyE, "input.key.e")
KEY(keyF, "input.key.f")
KEY(keyG, "input.key.g")
KEY(keyH, "input.key.h")
KEY(keyI, "input.key.i")
KEY(keyJ, "input.key.j")
KEY(keyK, "input.key.k")
KEY(keyL, "input.key.l")
KEY(keyM, "input.key.m")
KEY(keyN, "input.key.n")
KEY(keyO, "input.key.o")
KEY(keyP, "input.key.p")
KEY(keyQ, "input.key.q")
KEY(keyR, "input.key.r")
KEY(keyS, "input.key.s")
KEY(keyT, "input.key.t")
KEY(keyU, "input.key.u")
KEY(keyV, "input.key.v")
KEY(keyW, "input.key.w")
KEY(keyX, "input.key.x")
KEY(keyY, "input.key.y")
KEY(keyZ, "input.key.z")

KEY(key1, "input.key.1")
KEY(key2, "input.key.2")
KEY(key3, "input.key.3")
KEY(key4, "input.key.4")
KEY(key5, "input.key.5")
KEY(key6, "input.key.6")
KEY(key7, "input.key.7")
KEY(key8, "input.key.8")
KEY(key9, "input.key.9")
KEY(key0, "input.key.0")

KEY(keyLeft, "input.key.left")
KEY(keyRight, "input.key.right")
KEY(keyUp, "input.key.up")
KEY(keyDown, "input.key.down")

KEY(keyLeftMouse, "input.key.left-mouse")
KEY(keyRightMouse, "input.key.right-mouse")
KEY(keyMiddleMouse, "input.key.middle-mouse")

KEY(keyLeftShift, "input.key.left-shift")
KEY(keyRightShift, "input.key.right-shift")
KEY(keyLeftControl, "input.key.left-ctrl")
KEY(keyRightControl, "input.key.right-ctrl")
KEY(keyLeftAlt, "input.key.left-alt")
KEY(keyRightAlt, "input.key.right-alt")

KEY(keySpace, "input.key.space")
KEY(keyTilde, "input.key.tilde")
KEY(keyEsc, "input.key.esc")

// gamepad input
KEY(padDirectionLeft, "input.pad.left")
KEY(padDirectionRight, "input.pad.right")
KEY(padDirectionUp, "input.pad.up")
KEY(padDirectionDown, "input.pad.down")

// letter keys on xbox, shape keys on ps4
KEY(padButtonLeft, "input.button.left")
KEY(padButtonRight, "input.button.right")
KEY(padButtonUp, "input.button.up")
KEY(padButtonDown, "input.button.down")

// top buttons
KEY(padBack, "input.button.back")
KEY(padStart, "input.button.start")

// pressing down on thumbsticks
KEY(padLeftStick, "input.button.left-thumb")
KEY(padRightStick, "input.button.right-thumb")

KEY(padLeftBumper, "input.button.left-bumper")
KEY(padRightBumper, "input.button.right-bumper")

#undef KEY

#ifndef AXIS
#   define AXIS(id, name)
#endif

// mouse input axis
AXIS(mouseHorizontal, "input.axis.mouse.horizontal")
AXIS(mouseVertical, "input.axis.mouse.vertical")

// left joystick axis
AXIS(padLeftStickHorizontal, "input.axis.left-stick.horizontal")
AXIS(padLeftStickVertical, "input.axis.left-stick.vertical")

// right joystick axis
AXIS(padRightStickHorizontal, "input.axis.right-stick.horizontal")
AXIS(padRightStickVertical, "input.axis.left-stick.vertical")

AXIS(padLeftTrigger, "input.axis.left-trigger")
AXIS(padRightTrigger, "input.axis.right-trigger")

#undef AXIS
