// The gathering of the DICOM files of a folder for the explorer (DICOMgather.m): their attributes, the people
// (included by Shell.h, after the headers of the parts before it)
#pragma once

// The values of the attributes of all the files, each text once (the files of a series share most)
class ValuePool
{
public:
    Uint32 Id(const std::wstring& v)
    {
        AcquireSRWLockExclusive(&lock);
        Uint32 id = 0;
        auto it = ids.find(v);
        if (it != ids.end()) id = it->second;
        else
        {
            id = static_cast<Uint32>(texts.size());
            texts.push_back(v);
            ids.emplace(v, id);
        }
        ReleaseSRWLockExclusive(&lock);
        return id;
    }
    // (once the gathering is over: nothing writes then)
    const std::wstring& Text(Uint32 id) const { return texts[id]; }

private:
    SRWLOCK lock = SRWLOCK_INIT;
    std::unordered_map<std::wstring, Uint32> ids;
    std::vector<std::wstring> texts;
};

using Attributes = std::vector<std::pair<Uint32, Uint32>>;      // (tag, its value in the pool), by tag

// A DICOM file of the explorer: what places it in the tree, and all its attributes
struct Gathered
{
    std::wstring path;
    std::string charset, patient, patientName, birthDate, sex, age, study, studyDate, studyTime, studyDescription;
    std::string series, seriesNumber, seriesDate, seriesTime, seriesDescription, modality, acquisitionDate;
    std::string instance, trigger, time, orientationText, positionText;
    double orientation[6] = {}, position[3] = {}, z = 0;
    bool oriented = false, placed = false, hasZ = false;
    unsigned rows = 0, columns = 0;
    long frames = 1;                                // (a frame of an enhanced file: all of them in its first one, 0 in the rest)
    int frame = -1;                                 // a frame of an enhanced multi-frame file (-1: the whole file)
    long long bytes = 0;                            // (a frame: the size of the file in its first one, 0 in the rest)
    Attributes attributes;
};

// Who the patient of a file is, beyond its PatientID: its sex, its year of birth and the words of its
// name, in capitals, without accents and in alphabetical order ("Garcia^Juan", "JUAN GARCIA" and "garcia,
// juan" are the same name, with an accent on the i too). Some exports put the same placeholder in the
// PatientID of everyone (9999), and files without one all have the same empty one: these tell their
// people apart. Placeholders are left out: a birth date after the study or before 1900, a name such as
// ANONYMOUS or PATIENT (the same as DICOMstack, whose .Person and PersonHash these are).
struct Person
{
    std::string sex;                                // M, F or O ("": not said)
    std::string birth;                              // the birth date (its digits), when it is a real one
    int ageYear = 0;                                // the year of the study less PatientAge (0: not known)
    int year = 0;                                   // of birth: of the birth date, or else from the age (0: not known)
    bool yearOfBirthDate = false;                   // the year from PatientBirthDate (not from PatientAge)
    std::string rawName, charset;                   // the name as read, when it is one (not a placeholder)
    std::wstring name;                              // its words, in order, one space between them
    std::vector<std::wstring> words;
    bool Empty() const { return sex.empty() && !year && name.empty(); }
    int Known() const { return !sex.empty() + (year != 0) + !name.empty(); }
};

Person PersonOf(const Gathered& g);

void InheritInStudy(std::vector<Person*>& study);

// The people of files of one PatientID (in their order), each value a file does not say taken from the
// other files of its study when they agree on it
std::vector<Person> PeopleOf(const std::vector<Gathered>& files, const std::vector<int>& group);

bool SamePerson(const Person& a, const Person& b);

// 'Pid<PatientID>-<sex>-<year of birth>', '_' for what is not known (never the name): 'Pid12345-F-1970'
std::wstring PersonText(const std::wstring& patientId, const Person& p);

// The hash of a person (of its PatientID, the people apart): that of DICOMstack
std::wstring PersonHash(const Person& p);

enum class Found { Other, Dicom, Directory };

// The files under a folder, found and read in a thread of its own (and the reading in a few more)
struct Gathering
{
    std::wstring root;
    HWND window = nullptr;
    UINT message = 0;
    std::atomic<bool> cancel{ false }, listing{ true };
    std::atomic<long> found{ 0 }, done{ 0 };
    std::vector<Gathered> files;                    // the DICOM ones (written before the message)
    ValuePool pool;
    long others = 0, directories = 0, offline = 0;
    double seconds = 0;
    std::thread thread;
};

void Gather(Gathering* g);

enum class Level { All, Patient, Study, Series, Orientation, Position, Image };

// A node of the tree: the files under it (in the order of the tree), its children, and those shown in
// the tree (compact: orientations and positions that add nothing are skipped)
struct GatherNode
{
    Level level = Level::All;
    int parent = -1, shownParent = -1;
    std::vector<int> children, shown, files;
    std::wstring text;
    HTREEITEM item = nullptr;
    bool filled = false;                            // its shown children are in the tree
    bool hasCommon = false;
    Attributes common;                              // the attributes all its files share
    std::vector<std::wstring> problems, flags;      // what the checks found in it: in full, and in a word or two
    std::vector<std::wstring> notes;                // what they found that is not a problem by itself
    std::wstring person;                            // a patient: a hash of who it is
    std::wstring who;                               // a patient: who it is, 'Pid12345-F-1970' (PersonText)
    int sharing = 1;                                // a patient: how many people have its PatientID
};

bool IdentifyingTag(Uint32 tag);

std::wstring TagText(Uint32 tag);

std::wstring TagName(Uint32 tag);

std::string Narrow(const std::wstring& w);

void CopyText(HWND owner, const std::wstring& text);

void ShowInExplorer(const std::vector<std::wstring>& paths);
