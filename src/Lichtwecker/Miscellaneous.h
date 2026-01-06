#ifndef INC_MISCELLANEOUS_H
#define INC_MISCELLANEOUS_H

#define DEF_SINGLETON( NAME )    \
 public:                         \
    static NAME& GetInstance()   \
    {                            \
       static NAME _instance;    \
       return _instance;         \
    }                            \
 private:                        \
    NAME();                      \
    NAME( const NAME& );



template <typename C>
 class Singleton
 {
 public:
    static C* GetInstance ()
    {
       if (!_instance)
          _instance = new C ();
       return _instance;
    }
    virtual
    ~Singleton ()
    {
       delete _instance;
       _instance = 0;
    }
 private:
    static C* _instance;
 protected:
    Singleton () { }
 };
 template <typename C> C* Singleton <C>::_instance = 0;



#endif
