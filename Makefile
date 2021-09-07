#subsystem:
#         $(MAKE) -C subdir
#
#DIRS = libraries main

#evidence: 
#$(ECHO) looking into evidence : $(MAKE) $(MFLAGS)
#cd libraries/evidence; $(MAKE) $(MFLAGS)

#$(ECHO) looking into basictools : $(MAKE) $(MFLAGS)
#cd libraries/basictools; $(MAKE) $(MFLAGS)

#$(ECHO) looking into sleuthkit : $(MAKE) $(MFLAGS)
#cd libraries/sleuthkit; $(MAKE) $(MFLAGS)

#$(ECHO) looing into main : $(MAKE) $(MFLAGS)
#cd main; $(MAKE) $(MFLAGS)

wombattools:
	$(MAKE) -C wombatexport

	$(MAKE) -C wombatfuse

	$(MAKE) -C wombatimager

	$(MAKE) -C wombatinfo

	$(MAKE) -C wombatlogical

	$(MAKE) -C wombatverify
