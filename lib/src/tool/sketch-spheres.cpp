/* This file is part of Dilay
 * Copyright © 2015,2016 Alexander Bau
 * Use and redistribute under the terms of the GNU General Public License
 */
#include <QObject>
#include <QWheelEvent>
#include "cache.hpp"
#include "camera.hpp"
#include "config.hpp"
#include "dimension.hpp"
#include "scene.hpp"
#include "primitive/plane.hpp"
#include "primitive/ray.hpp"
#include "primitive/sphere.hpp"
#include "sketch/mesh.hpp"
#include "sketch/mesh-intersection.hpp"
#include "sketch/path.hpp"
#include "sketch/path-intersection.hpp"
#include "state.hpp"
#include "tools.hpp"
#include "util.hpp"
#include "view/cursor.hpp"
#include "view/pointing-event.hpp"
#include "view/tool-tip.hpp"

namespace {
  int toInt (SketchPathSmoothEffect effect) {
    switch (effect) {
      case SketchPathSmoothEffect::None:           return 0;
      case SketchPathSmoothEffect::Embed:          return 1;
      case SketchPathSmoothEffect::EmbedAndAdjust: return 2;
      case SketchPathSmoothEffect::Pinch:          return 3;
      default: DILAY_IMPOSSIBLE
    }
  }

  SketchPathSmoothEffect toSmoothEffect (int i) {
    switch (i) {
      case 0:  return SketchPathSmoothEffect::None;
      case 1:  return SketchPathSmoothEffect::Embed;
      case 2:  return SketchPathSmoothEffect::EmbedAndAdjust;
      case 3:  return SketchPathSmoothEffect::Pinch;
      default: DILAY_IMPOSSIBLE
    }
  }
}

struct ToolSketchSpheres::Impl {
  ToolSketchSpheres*     self;
  ViewCursor             cursor;
  float                  radius;
  float                  radiusStep;
  float                  height;
  SketchPathSmoothEffect smoothEffect;
  float                  stepWidthFactor;
  glm::vec3              previousPosition;
  SketchMesh*            mesh;

  Impl (ToolSketchSpheres* s)
    : self            (s)
    , radius          (s->cache ().get <float> ("radius", 0.1f))
    , radiusStep      (0.1f)
    , height          (s->cache ().get <float> ("height", 0.2f))
    , smoothEffect    (toSmoothEffect (s->cache ().get <int> 
						("smoothEffect", toInt (SketchPathSmoothEffect::Embed))))
    , stepWidthFactor (0.0f)
    , mesh            (nullptr)
  {}

  void setupToolTip () {
    ViewToolTip toolTip;
    toolTip.add ( ViewToolTip::MouseEvent::Left, QObject::tr ("Drag to sketch"));
    toolTip.add ( ViewToolTip::MouseEvent::Left, ViewToolTip::Modifier::Shift
                , QObject::tr ("Drag to smooth"));
    toolTip.add ( ViewToolTip::MouseEvent::Wheel, ViewToolTip::Modifier::Shift
                , QObject::tr ("Change radius") );
    this->self->showToolTip (toolTip);
  }

  void setupCursor () {
    this->cursor.disable ();
    this->cursor.radius  (this->radius);
  }

  ToolResponse runInitialize () {
    this->self->renderMirror (false);

    // this->setupProperties ();
    this->setupToolTip    ();
    this->setupCursor     ();

    return ToolResponse::Redraw;
  }

  void runRender () const {
    Camera& camera = this->self->state ().camera ();

    if (this->cursor.isEnabled ()) {
      this->cursor.render (camera);
    }
  }

  glm::vec3 newSpherePosition (bool considerHeight, const SketchMeshIntersection& intersection) {
    if (considerHeight) {
      return intersection.position () 
           - (intersection.normal () * float (this->radius
                                     * (1.0f - (2.0f * this->height))));
    }
    else {
      return intersection.position ();
    }
  }

  ToolResponse runMoveEvent (const ViewPointingEvent& e) {
    auto minDistance = [this] (const Intersection& intersection) -> bool {
      const float d = this->radius * this->stepWidthFactor;
      return glm::distance (this->previousPosition, intersection.position ()) > d;
    };

    if (e.primaryButton ()) {
      if (e.modifiers () == Qt::ShiftModifier) {
        SketchPathIntersection intersection;

        if (this->self->intersectsScene (e, intersection)) {
          this->cursor.enable   ();
          this->cursor.position (intersection.position ());

          if (minDistance (intersection)) {
            this->previousPosition = intersection.position ();
            this->mesh             = &intersection.mesh ();

            this->mesh->smoothPath ( intersection.path ()
                                   , PrimSphere ( intersection.position ()
                                                , this->radius )
                                   , 1
                                   , this->smoothEffect
                                   , this->self->mirrorDimension () );
          }
        }
      }
      else if (this->mesh) {
        SketchMeshIntersection intersection;
        const unsigned int     numExcludedLastPaths = this->self->hasMirror () ? 2 : 1;
              bool             considerHeight       = true;

        if (this->self->intersectsScene (e, intersection, numExcludedLastPaths) == false) {
          const Camera&   camera = this->self->state ().camera ();
          const PrimRay   ray    = camera.ray (e.ivec2 ());
          const PrimPlane plane  = PrimPlane ( this->previousPosition
                                             , DimensionUtil::vector (camera.primaryDimension ()) );
          float t;
          if (IntersectionUtil::intersects (ray, plane, &t)) {
            intersection.update (t, ray.pointAt (t), plane.normal (), *this->mesh);
            considerHeight = false;
          }
        }

        if (intersection.isIntersection () && minDistance (intersection)) {
          this->previousPosition = intersection.position ();

          this->mesh->addSphere ( false
                                , intersection.position ()
                                , this->newSpherePosition (considerHeight, intersection)
                                , this->radius
                                , this->self->mirrorDimension () );
        }
      }
      return ToolResponse::Redraw;
    }
    else {
      return this->runCursorUpdate (e.ivec2 ());
    }
  }

  ToolResponse runPressEvent (const ViewPointingEvent& e) {
    auto setupOnIntersection = [this] (const SketchMeshIntersection& intersection) {
      this->cursor.enable   ();
      this->cursor.position (intersection.position ());

      this->self->snapshotSketchMeshes ();

      this->mesh             = &intersection.mesh ();
      this->previousPosition = intersection.position ();
    };

    if (e.primaryButton ()) {
      if (e.modifiers () == Qt::ShiftModifier) {
        SketchPathIntersection intersection;
        if (this->self->intersectsScene (e, intersection)) {
          setupOnIntersection (intersection);

          this->mesh->smoothPath ( intersection.path ()
                                 , PrimSphere ( intersection.position ()
                                              , this->radius )
                                 , 1
                                 , this->smoothEffect
                                 , this->self->mirrorDimension () );
        }
      }
      else {
        SketchMeshIntersection intersection;
        if (this->self->intersectsScene (e, intersection)) {
          setupOnIntersection (intersection);

          this->mesh->addSphere ( true
                                , intersection.position ()
                                , this->newSpherePosition (true, intersection)
                                , this->radius
                                , this->self->mirrorDimension () );
        }
        else {
          this->cursor.disable ();
        }
      }
      return ToolResponse::Redraw;
    }
    else {
      return ToolResponse::None;
    }
  }

  ToolResponse runReleaseEvent (const ViewPointingEvent&) {
    this->mesh = nullptr;
    return ToolResponse::None;
  }

  ToolResponse runWheelEvent (const QWheelEvent& e) {
    if (e.orientation () == Qt::Vertical && e.modifiers () == Qt::ShiftModifier) {
      if (e.delta () > 0) {
        this->radius = this->radius + this->radiusStep;
      }
      else if (e.delta () < 0) {
        this->radius = this->radius - this->radiusStep;
      }
    }
    return ToolResponse::Redraw;
  }

  ToolResponse runCursorUpdate (const glm::ivec2& pos) {
    SketchMeshIntersection intersection;
    if (this->self->intersectsScene (pos, intersection)) {
      this->cursor.enable   ();
      this->cursor.position (intersection.position ());
    }
    else {
      this->cursor.disable ();
    }
    return ToolResponse::Redraw;
  }

  void runFromConfig () {
    const Config& config = this->self->config ();

    this->cursor.color (config.get <Color> ("editor/tool/sketchSpheres/cursorColor"));
    this->stepWidthFactor = config.get <float> ("editor/tool/sketchSpheres/stepWidthFactor");
  }
};

DELEGATE_TOOL                       (ToolSketchSpheres)
DELEGATE_TOOL_RUN_INITIALIZE        (ToolSketchSpheres)
DELEGATE_TOOL_RUN_RENDER            (ToolSketchSpheres)
DELEGATE_TOOL_RUN_MOVE_EVENT        (ToolSketchSpheres)
DELEGATE_TOOL_RUN_PRESS_EVENT       (ToolSketchSpheres)
DELEGATE_TOOL_RUN_RELEASE_EVENT     (ToolSketchSpheres)
DELEGATE_TOOL_RUN_MOUSE_WHEEL_EVENT (ToolSketchSpheres)
DELEGATE_TOOL_RUN_CURSOR_UPDATE     (ToolSketchSpheres)
DELEGATE_TOOL_RUN_FROM_CONFIG       (ToolSketchSpheres)

void ToolSketchSpheres::syncMirror()
{
    mirrorWingedMeshes ();
    updateGlWidget ();
}

float ToolSketchSpheres::radius() const
{
    return impl->radius;
}
void ToolSketchSpheres::radius(float f)
{
    impl->radius = f;
    impl->cursor.radius (f);
    cache ().set ("radius", f);
}

float ToolSketchSpheres::height() const
{
    return impl->height;
}
void ToolSketchSpheres::height(float f)
{
    impl->height = f;
    cache ().set ("height", f);
}

SketchPathSmoothEffect ToolSketchSpheres::smoothEffect() const
{
    return impl->smoothEffect;
}
void ToolSketchSpheres::smoothEffect(SketchPathSmoothEffect f)
{
    impl->smoothEffect = f;
    cache ().set ("smoothEffect", toInt(f));
}

