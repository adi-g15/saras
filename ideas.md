* add option to #include or import a name (for eg. "import notes as 0"), which
  requires a 'notes.module' file with 'lines' of data

  then that can be added in main as:

  # note = getimport(0)   # This should create a function getnote() that uses that
                        # '0' ie. notes import
  cs5404 = getnotes(4)

  import secrets
  abd_didnt_want_to_retire = getsecrets(6)

  import sanskrit.veda    # sanskrit.module is a json file, and 'veda' key has
                          # value again as 'array of line
  tran = getveda(3)

  Implemetation:
  When encountered "import notes", then add a functionAST for
  getnotes(double/*preferably string when supported*/),

  And add "extern fopen, fclose" etc, _then use a C function_ to return that line
  number as passed in the function (ensure it is a whole number), since the
  lexer parser code is the one running, so I guess we can do it
  
  These modules will be searched in include directories passed with '-I' flag

* Support Hindi

https://en.wikipedia.org/wiki/UTF-8#Encoding
