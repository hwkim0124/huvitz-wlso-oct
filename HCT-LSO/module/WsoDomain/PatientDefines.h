#pragma once

#include "WsoDomain2.h"

namespace wso_domain
{
	enum class EyeSide : int {
		UNKNOWN = 0,
		OD = 1,
		OS = 2,
		BOTH = 3
	};

	enum class EyeRegion : int {
		UNKNOWN = 0,
		MACULAR = 1,
		OPTIC_DISC = 2,
		FUNDUS = 3,
		CORNEA = 4, 
		MIRROR = 5
	};

	enum class FixationTarget : int {
		UNKNOWN = 0,
		CENTER = 1,
		OPTIC_DISC = 2,
		FUNDUS = 3,
		LEFT_SIDE = 4,
		LEFT_TOP = 5,
		LEFT_BOTTOM = 6,
		RIGHT_SIDE = 7,
		RIGHT_TOP = 8,
		RIGHT_BOTTOM = 9
	};

	enum class Ethinicity : int {
		UNKNOWN = 0,
		ASIAN = 1,
		COCASIAN = 2,
		LATINO = 3,
		MIXED = 7,
	};

	enum class Gender : int
	{
		UNKNOWN = 0,
		MALE = 1,
		FEMALE = 2
	};


	struct OctPatient
	{
	public:
		std::string _id = "";
		std::string _firstName = "";
		std::string _middleName = "";
		std::string _lastName = "";

		std::string _birthDate = "";
		std::uint16_t _age = 0;
		Gender _gender = Gender::UNKNOWN;

		float _diopterOd = 0.0f;
		float _diopterOs = 0.0f;

	public:
		void setup(std::string id, std::string fName, std::string mName, std::string lName,
			std::string birth, std::uint16_t age, Gender gender, float dioptOd = 0.0f, float dioptOs = 0.0f) {
			_id = id;
			_firstName = fName;
			_middleName = mName;
			_lastName = lName;
			_birthDate = birth;
			_age = age;
			_gender = gender;
			_diopterOd = dioptOd;
			_diopterOs = dioptOs;
			return;
		}

		void init(void) {
			setup("", "", "", "", "", 0, Gender::UNKNOWN, 0.0f, 0.0f);
		}

		float diopterOD(void) {
			return _diopterOd;
		}

		float diopterOS(void) {
			return _diopterOs;
		}

		void setDiopter(float dioptOd, float dioptOs) {
			_diopterOd = dioptOd;
			_diopterOs = dioptOs;
			return;
		}
	};
}