//************************************ bs::framework - Copyright 2018 Marko Pintera **************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#pragma once

#include "BsCorePrerequisites.h"
#include "Particles/BsParticleSystem.h"
#include "Scene/BsComponent.h"

namespace bs 
{
	/** @addtogroup Components-Core
	 *  @{
	 */

	/**
	 * @copydoc	ParticleSystem
	 *
	 * @note Wraps ParticleSystem as a Component.
	 */
	class BS_CORE_EXPORT CParticleSystem : public Component
	{
	public:
		CParticleSystem(const HSceneObject& parent);
		virtual ~CParticleSystem() = default;
		
		/** @copydoc ParticleSystem::setSettings */
		void setSettings(const ParticleSystemSettings& settings);

		/** @copydoc ParticleSystem::getSettings */
		const ParticleSystemSettings& getSettings() const { return mSettings; }

		/** @copydoc ParticleSystem::getEmitters */
		ParticleSystemEmitters& getEmitters();

		/** @copydoc ParticleSystem::getEvolvers */
		ParticleSystemEvolvers& getEvolvers();

		/** @name Internal
		 *  @{
		 */

		/** Returns the ParticleSystem implementation wrapped by this component. */
		ParticleSystem* _getInternal() const { return mInternal.get(); }

		/** @} */

		/************************************************************************/
		/* 						COMPONENT OVERRIDES                      		*/
		/************************************************************************/
	protected:
		friend class SceneObject;

		/** @copydoc Component::onDestroyed() */
		void onDestroyed() override;

		/** @copydoc Component::onDisabled() */
		void onDisabled() override;

		/** @copydoc Component::onEnabled() */
		void onEnabled() override;

	protected:
		using Component::destroyInternal;

		/** Creates the internal representation of the ParticleSystem and restores the values saved by the Component. */
		void restoreInternal();

		/** Destroys the internal ParticleSystem representation. */
		void destroyInternal();

		SPtr<ParticleSystem> mInternal;

		ParticleSystemSettings mSettings;
		ParticleSystemEmitters mEmitters;
		ParticleSystemEvolvers mEvolvers;

		/************************************************************************/
		/* 								RTTI		                     		*/
		/************************************************************************/
	public:
		friend class CParticleSystemRTTI;
		static RTTITypeBase* getRTTIStatic();
		RTTITypeBase* getRTTI() const override;

	protected:
		CParticleSystem(); // Serialization only
	 };

	 /** @} */
}