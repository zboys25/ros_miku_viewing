




#include "rviz_sphere_view_controller/rviz_sphere_view_controller.h"

#include <OGRE/OgreViewport.h>
#include <OGRE/OgreQuaternion.h>
#include <OGRE/OgreVector3.h>
#include <OGRE/OgreSceneNode.h>
#include <OGRE/OgreSceneManager.h>
#include <OGRE/OgreCamera.h>

#include "rviz/uniform_string_stream.h"
#include "rviz/display_context.h"
#include "rviz/viewport_mouse_event.h"
#include "rviz/geometry.h"
#include "rviz/ogre_helpers/shape.h"
#include "rviz/properties/float_property.h"
#include "rviz/properties/vector_property.h"


namespace rviz_sphere_view_controller {
using namespace rviz;

static const Ogre::Quaternion ROBOT_TO_CAMERA_ROTATION =
    Ogre::Quaternion(Ogre::Radian(-Ogre::Math::HALF_PI), Ogre::Vector3::UNIT_Y) *
    Ogre::Quaternion(Ogre::Radian(-Ogre::Math::HALF_PI), Ogre::Vector3::UNIT_Z);

static const float PITCH_LIMIT_LOW = -Ogre::Math::HALF_PI + 0.001;
static const float PITCH_LIMIT_HIGH = Ogre::Math::HALF_PI - 0.001;

SphereViewController::SphereViewController() {
    distance_property_ = new FloatProperty("Distance", 0, "The distance to the target frame.", this);
    distance_property_->setMin(0.001);
    pos_offset_property_ = new VectorProperty("Pos Offset", Ogre::Vector3::ZERO, "Position offset from the target frame.", this);
    ori_offset_property_ = new VectorProperty("Ori Offset[deg]", Ogre::Vector3::ZERO, "Orientation offset from the target frame.", this);
    camera_position_ = Ogre::Vector3(-distance_property_->getFloat(), 0.0, 0.0);
    camera_orientation_ = Ogre::Quaternion::IDENTITY;

}

SphereViewController::~SphereViewController() {
}

void SphereViewController::onInitialize() {
    FramePositionTrackingViewController::onInitialize();
    camera_->setProjectionType(Ogre::PT_PERSPECTIVE);
}

void SphereViewController::reset() {
    camera_position_ = Ogre::Vector3(-distance_property_->getFloat(), 0.0, 0.0);
    camera_orientation_ = Ogre::Quaternion::IDENTITY;
    updateCamera();
}

void SphereViewController::handleMouseEvent(ViewportMouseEvent& event) {
    setStatus("<b>Left-Click:</b> Move YAW/PITCH.  <b>Wheel:</b>: Move Distance.");

    bool moved = false;

    int32_t diff_x = 0;
    int32_t diff_y = 0;

    if (event.type == QEvent::MouseMove) {
        diff_x = event.x - event.last_x;
        diff_y = event.y - event.last_y;
        if (event.left()) {
            setCursor(Rotate3D);
            camera_orientation_ = camera_orientation_ *  Ogre::Quaternion(Ogre::Radian(-diff_x * 0.005), Ogre::Vector3::UNIT_Z);
            camera_orientation_ = camera_orientation_ *  Ogre::Quaternion(Ogre::Radian(diff_y * 0.005), Ogre::Vector3::UNIT_Y);
        }
        moved = true;
    }

    if (event.wheel_delta != 0) {
        camera_position_.x -= event.wheel_delta;
        moved = true;
    }

    if (moved) {
        context_->queueRender();
    }
}


void SphereViewController::mimic(ViewController* source_view) {
    FramePositionTrackingViewController::mimic(source_view);
}

void SphereViewController::update(float dt, float ros_dt) {
    FramePositionTrackingViewController::update(dt, ros_dt);
    updateCamera();
}

void SphereViewController::lookAt(const Ogre::Vector3& point) {
    camera_->lookAt(point);
}

void SphereViewController::onTargetFrameChanged(const Ogre::Vector3& old_reference_position, const Ogre::Quaternion& old_reference_orientation) {
    position_property_->add(old_reference_position - reference_position_);
}

void SphereViewController::updateCamera() {

    pos_offset = pos_offset_property_->getVector();
    ori_offset = ori_offset_property_->getVector();
    camera_->lookAt(0, 0, 0);
    camera_->setOrientation(camera_orientation_ + pos_offset);
    camera_->setPosition(camera_position_);
}

Ogre::Quaternion SphereViewController::getOrientation() {
    Ogre::Quaternion pitch, yaw;

    yaw.FromAngleAxis(Ogre::Radian(yaw_property_->getFloat()), Ogre::Vector3::UNIT_Z);
    pitch.FromAngleAxis(Ogre::Radian(pitch_property_->getFloat()), Ogre::Vector3::UNIT_Y);

    return yaw * pitch * ROBOT_TO_CAMERA_ROTATION;
}

void SphereViewController::move(float x, float y, float z) {
    Ogre::Vector3 translate(x, y, z);
    position_property_->add(getOrientation() * translate);
}

} // end namespace rviz_sphere_view_controller

#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS(rviz_sphere_view_controller::SphereViewController, rviz::ViewController)
