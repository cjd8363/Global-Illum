struct sourceManager * 
dsCreateSource(const char * const fileName);

void
dsDestroySource(struct sourceManager * const srcP);

bool
dsDataLeft(struct sourceManager * const srcP);

struct jpeg_source_mgr *
dsJpegSourceMgr(struct sourceManager * const srcP);
