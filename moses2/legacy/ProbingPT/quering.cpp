#include "quering.hh"

namespace Moses2
{

unsigned char * read_binary_file(const char * filename, size_t filesize)
{
  //Get filesize
  int fd;
  unsigned char * map;

  fd = open(filename, O_RDONLY);

  if (fd == -1) {
    perror("Error opening file for reading");
    exit(EXIT_FAILURE);
  }

  map = (unsigned char *)mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    close(fd);
    perror("Error mmapping the file");
    exit(EXIT_FAILURE);
  }

  return map;
}

QueryEngine::QueryEngine(const char * filepath) : decoder(filepath)
{

  //Create filepaths
  std::string basepath(filepath);
  std::string path_to_hashtable = basepath + "/probing_hash.dat";
  std::string path_to_data_bin = basepath + "/binfile.dat";
  std::string path_to_source_vocabid = basepath + "/source_vocabids";

  ///Source phrase vocabids
  read_map(&source_vocabids, path_to_source_vocabid.c_str());

  //Target phrase vocabIDs
  vocabids = &decoder.get_target_lookup_map();

  //Read config file
  std::string line;
  std::ifstream config ((basepath + "/config").c_str());
  //Check API version:
  getline(config, line);
  if (atoi(line.c_str()) != API_VERSION) {
    std::cerr << "The ProbingPT API has changed, please rebinarize your phrase tables." << std::endl;
    exit(EXIT_FAILURE);
  }
  //Get tablesize.
  getline(config, line);
  int tablesize = atoi(line.c_str());
  //Number of scores
  getline(config, line);
  num_scores = atoi(line.c_str());
  //How may scores from lex reordering models
  getline(config, line);
  num_lex_scores = atoi(line.c_str());
  // have the scores been log() and FloorScore()?
  getline(config, line);
  logProb = atoi(line.c_str());

  config.close();

  //Mmap binary table
  struct stat filestatus;
  stat(path_to_data_bin.c_str(), &filestatus);
  binary_filesize = filestatus.st_size;
  binary_mmaped = read_binary_file(path_to_data_bin.c_str(), binary_filesize);

  //Read hashtable
  table_filesize = Table::Size(tablesize, 1.2);
  mem = readTable(path_to_hashtable.c_str(), table_filesize);
  Table table_init(mem, table_filesize);
  table = table_init;

  std::cerr << "Initialized successfully! " << std::endl;
}

QueryEngine::~QueryEngine()
{
  //Clear mmap content from memory.
  munmap(binary_mmaped, binary_filesize);
  munmap(mem, table_filesize);

}

uint64_t QueryEngine::getKey(uint64_t source_phrase[], size_t size) const
{
  //TOO SLOW
  //uint64_t key = util::MurmurHashNative(&source_phrase[0], source_phrase.size());
  uint64_t key = 0;
  for (size_t i = 0; i < size; i++) {
	key += (source_phrase[i] << i);
  }
  return key;
}

std::pair<bool, std::vector<target_text*> > QueryEngine::query(uint64_t source_phrase[],
		size_t size,
		RecycleData &recycler)
{
  //TOO SLOW
  //uint64_t key = util::MurmurHashNative(&source_phrase[0], source_phrase.size());
  uint64_t key = getKey(source_phrase, size);

  std::pair<bool, std::vector<target_text*> > output = query(key, recycler);
  return output;
}

std::pair<bool, std::vector<target_text*> > QueryEngine::query(uint64_t key, RecycleData &recycler)
{
  std::pair<bool, std::vector<target_text*> > output;

  const Entry * entry;
  output.first = table.Find(key, entry);

  if (output.first) {
    //The phrase that was searched for was found! We need to get the translation entries.
    //We will read the largest entry in bytes and then filter the unnecesarry with functions
    //from line_splitter
    uint64_t initial_index = entry -> GetValue();
    unsigned int bytes_toread = entry -> bytes_toread;

    //Get only the translation entries necessary
    output.second = decoder.full_decode_line(binary_mmaped + initial_index, bytes_toread, num_scores, num_lex_scores, recycler);

  }

  return output;

}

void QueryEngine::printTargetInfo(const std::vector<target_text> &target_phrases)
{
  int entries = target_phrases.size();

  for (int i = 0; i<entries; i++) {
    std::cout << "Entry " << i+1 << " of " << entries << ":" << std::endl;
    //Print text
    std::cout << getTargetWordsFromIDs(target_phrases[i].target_phrase, *vocabids) << "\t";

    //Print probabilities:
    for (int j = 0; j<target_phrases[i].prob.size(); j++) {
      std::cout << target_phrases[i].prob[j] << " ";
    }
    std::cout << "\t";

    //Print word_all1
    for (int j = 0; j<target_phrases[i].word_all1.size(); j++) {
      if (j%2 == 0) {
        std::cout << (short)(target_phrases[i].word_all1)[j] << "-";
      } else {
        std::cout << (short)(target_phrases[i].word_all1)[j] << " ";
      }
    }
    std::cout << std::endl;
  }
}

}
