#pragma once

class CommandLineArgs
{
public:
	CommandLineArgs();

	bool UseCPU() const { return m_useCPU; }
	bool UseGPU() const { return m_useGPU; }
	bool UseCPUandGPU() const { return m_useCPUandGPU; }

	const std::wstring& ModelPath() const { return m_modelPath; }
	void SetModelPath(std::wstring path) { m_modelPath = path; }
	const std::wstring& FolderPath() const { return m_folderPath; }
	UINT NumIterations() const { return m_numIterations; }
	std::wstring CsvFileName() { return m_csvFileName; }
	bool MetacommandsEnabled() const { return m_metacommandsEnabled; }

private:
	bool m_useCPU = false;
	bool m_useGPU = false;
	bool m_useCPUandGPU = false;
	std::wstring m_folderPath;
	std::wstring m_modelPath;
	std::wstring m_csvFileName;
	UINT m_numIterations = 1;
	bool m_metacommandsEnabled = false;
};