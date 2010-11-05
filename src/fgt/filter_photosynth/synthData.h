/*
 * The following code is slightly based on the C# application
 * SynthExport (http://synthexport.codeplex.com/) by Christoph Hausner
 */

#ifndef SYNTHDATA_H
#define SYNTHDATA_H

#include <QString>
#include <QScriptValue>
#include <QtSoapHttpTransport>
#include <assert.h>

typedef struct Point
{
  float _x;
  float _y;
  float _z;
  uchar _r;
  uchar _g;
  uchar _b;
} Point;

typedef struct Image
{
  int _ID;
  int _width;
  int _height;
  QString _url;
} Image;

/*
typedef struct CameraParameters
{
  int _imageID;
  qreal _posX;
  qreal _posY;
  qreal _posZ;
  qreal _rotX;
  qreal _rotY;
  qreal _rotZ;
  qreal _aspectRatio;
  qreal _focalLength;
  qreal _distortionRadius1;
  qreal _distortionRadius2;
} CameraParameters;
*/

class CameraParameters
{
public:
  enum Field
  {
    FIRST = 0,
    POS_X = FIRST,
    POS_Y,
    POS_Z,
    ROT_X,
    ROT_Y,
    ROT_Z,
    ASPECT_RATIO,
    FOCAL_LENGTH,
    LAST = FOCAL_LENGTH
  };

  void rotationFromNormalizedQuaternion();
  //Point3f getTranslation();
  //Matrix44f getRotation();

  inline qreal &operator [] (const int i)
  {
    assert(i >= 0 && i < 8);
    return _fields[i];
  }

  int _camID;
  int _imageID;
  qreal _fields[8];
  qreal _distortionRadius1;
  qreal _distortionRadius2;
};

/*
 * Represents a set of points
 */
class PointCloud : public QObject
{
public:
  PointCloud(int coordSysID, int binFileCount, QObject *parent = 0);

public:
  //the coordinate system id within the synth which this set belongs to
  int _coordinateSystem;
  //this is the n parameter in the points_m_n.bin files containing the synth point clounds
  //and tells how many files this cloud is split into
  int _binFileCount;
  int _numberOfPoints;
  QList<Point> _points;
};

/*
 * Represents an independent cluster of points within the synth,
 * it is identified by an ID, contains a point cloud and
 * has its own camera parameters
 */
class CoordinateSystem : public QObject
{
public:
  CoordinateSystem(int id, QObject *parent = 0);

public:
  //this is the m parameter in the points_m_n.bin files containing the synth point clounds
  int _id;
  bool _shouldBeImported;
  PointCloud *_pointCloud;
  QList<CameraParameters> _cameraParametersList;
};

/*
 * Represents a Synth
 */
class SynthData : public QObject
{
  Q_OBJECT

  static QtSoapHttpTransport transport;

public:
  //contains errors descriptions
  static const QString errors[];
  //contains the strings used by cb() funcion
  static const char *progress[];

  enum Errors
  {
    WRONG_URL = 0,
    WRONG_PATH,
    WEBSERVICE_ERROR,
    NEGATIVE_RESPONSE,
    UNEXPECTED_RESPONSE,
    WRONG_COLLECTION_TYPE,
    JSON_PARSING,
    EMPTY,
    READING_BIN_DATA,
    BIN_DATA_FORMAT,
    CREATE_DIR,
    SAVE_IMG,
    NO_ERROR,
    PENDING
  };

  enum Progress
  {
    WEB_SERVICE = 0,
    DOWNLOAD_JSON,
    PARSE_JSON,
    DOWNLOAD_BIN,
    LOADING_BIN,
    DOWNLOAD_IMG
  };

  SynthData(QObject *parent = 0);
  ~SynthData();
  bool isValid();

public:
  static SynthData *downloadSynthInfo(QString url, QString path);

private slots:
  void readWSresponse();
  void parseJsonString(QNetworkReply *httpResponse);
  void loadBinFile(QNetworkReply *httpResponse);
  void saveImages(QNetworkReply *httpResponse);

private:
  void parseImageMap(QScriptValue &map);
  void downloadJsonData(QString jsonURL);
  void downloadBinFiles();
  void downloadImages();

public:
  //this is the cid parameter taken from the url used to access the synth on photosynth.net
  QString _collectionID;
  //the base url of the binary files points_m_n.bin containing point clouds data
  QString _collectionRoot;
  //Each coordinate system is a different cluster of point in the synth
  QList<CoordinateSystem*> *_coordinateSystems;
  //a dictionary mapping images id to image representation
  QHash<int,Image> *_imageMap;
  //tells if this synth is valid, or if errors were encountered during the import process
  Errors _state;
  Progress _progress;
  //when a SynthData is instantiated _dataReady == false
  //until the data are downloaded from photosynth server
  bool _dataReady;
  //Number of images of this synth
  int _numImages;

private:
  //used to count how many responses to bin files requests have been processed
  //when _semaphore reaches 0 _dataReady can be set to true
  int _semaphore;
  //the images will be saved here
  QString _savePath;
};

/*
 * Represents the options of the import process
 */
class ImportSettings
{
public:
  enum ImportSource { WEB_SITE, ZIP_FILE };

  ImportSettings(ImportSource source, QString sourcePath, bool importPointClouds, bool importCameraParameters);

public:
  //specifies if the synth has to be downloaded from a url or loaded from a zip file on the filesystem
  ImportSettings::ImportSource _source;
  //can be the cid parameter taken from the synth url or a path on a filesystem
  QString _sourcePath;
  //specifies if the point clouds have to be imported
  bool _importPointClouds;
  //specifies if the camera parameters have to be imported
  bool _importCameraParameters;
};

/*********************
 * Utility functions *
 *********************/

int readCompressedInt(QIODevice *device, bool &error);
float readBigEndianSingle(QIODevice *device, bool &error);
unsigned short readBigEndianUInt16(QIODevice *device, bool &error);
void printPoint(Point *p);

#endif // SYNTHDATA_H