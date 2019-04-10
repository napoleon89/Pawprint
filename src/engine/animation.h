#ifndef ANIMATION_H
#define ANIMATION_H

#include <engine/math.h>
#include <engine/collections.h>

struct JointPose {
	Quat rotation;
	Vec3 translation;
	Vec3 scale;

	static JointPose interpolate(const JointPose &a, const JointPose &b, f32 t);

	inline Mat4 toMat4() {
		return Mat4::translate(translation) * Quat::toMat4(rotation) * Mat4::scale(scale);
	}
};

struct Joint {
	Mat4 local_mat;
	Mat4 world_mat;
	Mat4 inv_bind_mat;
	s32 parent_index;
	char *name;
};

struct AnimationChannel {
	JointPose *poses;
	f32 *frame_times;
	u32 frame_count;
	s32 joint_index;

	JointPose getPose(f32 t);
};

struct AnimationClip {
	AnimationChannel *channels;
	f32 duration_seconds;
	u32 channel_count;
};

struct Skeleton {
	Joint *joints;
	u32 num_joints;

	s32 getJointIndexFromName(const char *name);
	void applyAnimationClip(const AnimationClip *clip, f32 timer);
};

struct AnimationSequence {
	DynamicArray<AnimationClip *> clips;
	f32 current_timer;
	u32 current_index;
	f32 local_timer;

	void progress(f32 delta);
};

#endif // ANIMATION_H