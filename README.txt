
Authors:
Alex Johnston - awj150030
Braden Baikauskas - bdb150230


To compile and run project2.c:

  gcc project2.c
  ./a.out

there will be a prompt for commands, all the commands available are:

	Mount x 	-- mounts file at path x
	Ckfiletype n 	-- displays information about file with inode #n
	Filesize m 	-- displays information about file with inode #m
  initfs n1 n2 -- n1=[number of blocks in the system] n2=[number of inode blocks]
  cpin f1 f2 -- f1=[outside file being copied] f2=[V6 file destination]
  cpout f1 f2 -- f1=[V6 file being copied] f2=[outside destination]
  mkdir d -- Make directory at path "d"
  rm d -- Remove file at path "d"
	q 		-- Saves and quits
