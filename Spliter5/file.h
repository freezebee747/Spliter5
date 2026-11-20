#pragma once

#include <string>
#include <unordered_set>
#include <vector>

// 파일 목록 조회 인터페이스.
// 검사기와 함수들은 실제 파일 시스템이 아니라 이 인터페이스에 의존한다(의존성 역전).
// 덕분에 테스트에서는 가짜 파일 목록을 넣을 수 있다.
class IFileLookup {
public:
	virtual ~IFileLookup() = default;
	virtual const std::unordered_set<std::string>& Filenames() const = 0;

	bool Exists(const std::string& filename) const;
	// 글롭 패턴(*, ?, [])과 일치하는 파일 이름을 정렬해서 반환한다.
	std::vector<std::string> Glob(const std::string& pattern) const;
};

bool HasGlobChars(const std::string& text);
std::string GlobToRegex(const std::string& glob);

//파일 클래스 관리를 위한 observer 패턴
class IDirObserver {
public:
	virtual ~IDirObserver() = default;
	virtual void OnDirChanged(const std::string& new_dir) = 0;
};

//검색 디렉토리 처리를 위한 singleton 클래스
//디렉토리는 반드시 SetDir을 거쳐서만 바뀌므로 옵저버 알림이 누락되지 않는다.
class DirSingleton {
public:
	static DirSingleton& GetInstance();
	const std::string& GetDir() const { return dir_; }
	void SetDir(const std::string& dir);
	void AttachObserver(IDirObserver* observer);
	void DetachObserver(IDirObserver* observer);

	DirSingleton(const DirSingleton&) = delete;
	DirSingleton& operator=(const DirSingleton&) = delete;

private:
	DirSingleton() = default;
	void NotifyObservers();

	std::string dir_ = ".";
	std::vector<IDirObserver*> observers_;
};

// 검색 디렉토리의 파일 목록을 캐시한다. 디렉토리가 바뀌면 다시 읽는다.
class FileManagement : public IFileLookup, public IDirObserver {
public:
	FileManagement();
	~FileManagement() override;
	FileManagement(const FileManagement&) = delete;  // 옵저버로 등록된 this 포인터가 복사되면 안 된다
	FileManagement& operator=(const FileManagement&) = delete;

	void OnDirChanged(const std::string& new_dir) override;
	const std::unordered_set<std::string>& Filenames() const override { return filenames_; }

	const std::string& Dir() const { return dir_; }
	bool IsValidDir() const { return valid_dir_; }

private:
	void Rescan();

	std::string dir_;
	bool valid_dir_ = false;
	std::unordered_set<std::string> filenames_;
};
