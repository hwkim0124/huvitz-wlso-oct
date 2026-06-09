#pragma once

#include "OctAngio.h"


namespace oct_angio
{
	class AngioLayout;
	class AngioDecorr;

	class OCTANGIO_DLL_API AngioFile
	{
	public:
		AngioFile();
		virtual ~AngioFile();

		AngioFile(AngioFile&& rhs);
		AngioFile& operator=(AngioFile&& rhs);

	public:
		static bool saveAngioDataFile(const std::string dirPath, const std::string fileName, 
									oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr);
		static bool loadAngioDataFile(const std::string dirPath, const std::string fileName,
									oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr);

	protected:
		struct DecorrTable {
			std::vector<int> indice;
			std::vector<unsigned short> dc_vals;
			std::vector<unsigned short> df_vals;

			DecorrTable(int size) {
				initialize(size);
			}

			void initialize(int size) {
				indice.resize(size, 0);
				dc_vals.resize(size, 0);
				df_vals.resize(size, 0);
			}
		};

		static bool readDataHeader(std::ifstream& file, oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr);
		static bool readDecorrSizes(std::ifstream& file, std::vector<int>& dsizes, oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr);
		static bool readDecorrTables(std::ifstream& file, std::vector<int> dsizes, oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr);

		static std::vector<char> makeDataHeader(oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr);
		static std::vector<int> makeDecorrSizes(oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr);
		static AngioFile::DecorrTable makeDecorrTable(int index, int dsize, oct_angio::AngioLayout& layout, oct_angio::AngioDecorr& decorr);

	private:
		struct AngioFileImpl;
		std::unique_ptr<AngioFileImpl> d_ptr;
		AngioFileImpl& getImpl(void) const;
	};
}

