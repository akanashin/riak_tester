class Exception: public std::exception {
    std::string m_txt;
public:
    Exception(std::string const& txt): m_txt(txt) {}
    const char* what() const throw() { return m_txt.c_str(); }
};
