#include <engine/animation.h>

JointPose JointPose::interpolate(const JointPose &a, const JointPose &b, f32 t) {
	JointPose result = {};
	result.translation = Vec3::lerp(a.translation, b.translation, t);
	result.scale = Vec3::lerp(a.scale, b.scale, t);
	result.rotation = Quat::interpolate(a.rotation, b.rotation, t);
	return result;
}

s32 Skeleton::getJointIndexFromName(const char *name) {
	for(u32 joint_index = 0; joint_index < num_joints; joint_index++) {
		const Joint &joint = joints[joint_index];
		if(strcmp(joint.name, name) == 0) {
			return joint_index;
		}
	}
	return -1;
}

void Skeleton::applyAnimationClip(const AnimationClip *clip, f32 timer) {
	for(u32 i = 0; i < clip->channel_count; i++) {
		AnimationChannel &channel = clip->channels[i];
		if(channel.joint_index == -1) continue;
		Joint &joint = joints[channel.joint_index];
		JointPose new_pose = channel.getPose(timer);
		joint.local_mat = new_pose.toMat4();
	}

	for(u32 i = 1; i < num_joints; i++) {
		Joint &joint = joints[i];
		Mat4 parent_world = joints[joint.parent_index].world_mat;
		joint.world_mat = parent_world * joint.local_mat;
	}
}

JointPose AnimationChannel::getPose(f32 t) {
	s32 first_frame_index = -1;
	s32 second_frame_index = -1;
	f32 interp = 0.0f;

	for(u32 i = 0; i < frame_count-1; i++) {
		f32 frame_time = frame_times[i];
		f32 future_frame_time = frame_times[i+1];
		f32 delta = future_frame_time - frame_time;
		f32 point = t - frame_time;

		if(t >= frame_time && t <= future_frame_time) {
			first_frame_index = i;
			second_frame_index = i+1;
			interp = point / delta;
			break;
		}
	}

	JointPose result = poses[0];

	if(first_frame_index != -1 && second_frame_index != -1) {
		JointPose first_pose = poses[first_frame_index];
		JointPose second_pose = poses[second_frame_index];
		
		result = JointPose::interpolate(first_pose, second_pose, interp);
	}

	return result;
}

void AnimationSequence::progress(f32 delta) {
	f32 future_timer = local_timer + delta;
	const AnimationClip *current_clip = clips.getRefConst(current_index);
	if(local_timer >= current_clip->duration_seconds) {
		
	}
}