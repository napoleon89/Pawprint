#ifndef INPUT_H
#define INPUT_H

#include <engine/std.h>
#include <core/platform.h>
#include <engine/math.h>

struct ButtonState {
	bool is_down;
	bool was_down;	
};

struct MouseState {
	union {
		ButtonState buttons[3];
		struct {
			ButtonState left;	
			ButtonState middle;	
			ButtonState right;	
		};
	};
	s32 x, y;
};


struct InputManager {
	private:
		ButtonState buttons[2][(u32)Key::KeyCount];
		ButtonState (*old_buttons)[(u32)Key::KeyCount];
		ButtonState (*new_buttons)[(u32)Key::KeyCount];
		MouseState mouse_states[2];
		MouseState *old_mouse;
		MouseState *new_mouse;
		PlatformWindow *window;
		
	public:
		
		InputManager(PlatformWindow *w);
		
		bool isKeyDown(Key key);		
		bool isKeyDownOnce(Key key);

		// relative to window
		Vec2i getMousePosition();
		Vec2i getMouseDelta();
		bool isMouseButtonDown(MouseButton button);
		bool isMouseButtonDownOnce(MouseButton button);
		
		// relative to window
		void setMousePosition(Platform *platform, s32 x, s32 y);
		void processKeys(Platform *platform);
		void endFrame();
		f32 getScrollAxis(Platform *platform);
};

#endif // INPUT_H