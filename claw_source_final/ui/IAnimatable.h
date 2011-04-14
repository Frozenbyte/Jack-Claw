
#ifndef IANIMATABLE_H
#define IANIMATABLE_H

namespace ui
{
  class VisualObject;

  /**
   * Interface defining a animatable object.
   * @version 1.0, 23.10.2002
   * @author Jukka Kokkonen <jukka@frozenbyte.com>
   * @see Animator
   * @see VisualObject
   */
  class IAnimatable
  {
  public:
	  virtual ~IAnimatable() {}

    virtual VisualObject *getVisualObject() = 0;

    virtual void setAnimation(int animation) = 0;

    virtual int getAnimation() = 0;

    virtual void setBlendAnimation(int num, int animation) = 0;

    virtual int getBlendAnimation(int num) = 0;

		virtual void setAnimationSpeedFactor(float speedFactor) = 0;

		virtual float getAnimationSpeedFactor() = 0;

  };

}

#endif

