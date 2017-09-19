/*
Copyright c1997-2014 Trygve Isaacson. All rights reserved.
This file is part of the Code Vault version 4.1
http://www.bombaydigital.com/
License: MIT. See LICENSE.md in the Vault top level directory.
*/

/** @file */

#include "vsettings.h"

#include "vexception.h"
#include "vmemorystream.h"
#include "vtextiostream.h"
#include "vbinaryiostream.h"
#include "vchar.h"
#include "vbento.h"
#include "vfilewriter.h"

V_STATIC_INIT_TRACE

#undef strlen

static VString _INDENT_STRING("    ");

// VSettingsNode ------------------------------------------------------------------

VSettingsNode::VSettingsNode(VSettingsTag* parent, const VString& name, bool preferCDATA)
    : mParent(parent)
    , mName(name)
    , mPreferCDATA(preferCDATA)
    {
}

VSettingsNode::VSettingsNode(const VSettingsNode& other)
    : mParent(other.mParent)
    , mName(other.mName)
    , mPreferCDATA(other.mPreferCDATA)
    {
}

VSettingsNode& VSettingsNode::operator=(const VSettingsNode& other) {
    mParent = other.mParent;
    mName = other.mName;
    mPreferCDATA = other.mPreferCDATA;

    return *this;
}

const VSettingsNode* VSettingsNode::findNode(const VString& path) const {
    if (path.isEmpty()) {
        return this;
    }

    VString nextNodeName;
    VString theRemainder;

    VSettings::splitPathFirst(path, nextNodeName, theRemainder);

    if (theRemainder.isEmpty()) {
        VSettingsAttribute*    attribute = this->_findAttribute(nextNodeName);
        if (attribute != NULL) {
            return attribute;
        }
    }

    VSettingsTag* child = this->_findChildTag(nextNodeName);
    if (child != NULL) {
        return child->findNode(theRemainder);
    } else {
        return NULL;
    }
}

VSettingsNode* VSettingsNode::findMutableNode(const VString& path) {
    return const_cast<VSettingsNode*>(this->findNode(path)); // const_cast: NON-CONST WRAPPER
}

int VSettingsNode::countNodes(const VString& path) const {
    int     result = 0;
    VString leadingPath;
    VString lastNode;

    VSettings::splitPathLast(path, leadingPath, lastNode);

    const VSettingsNode* parent = this->findNode(leadingPath);

    if (parent != NULL) {
        result = parent->countNamedChildren(lastNode);
    }

    return result;
}

void VSettingsNode::deleteNode(const VString& path) {
    VString leadingPath;
    VString lastNode;

    VSettings::splitPathLast(path, leadingPath, lastNode);

    VSettingsNode* parent = this->findMutableNode(leadingPath);

    if (parent != NULL) {
        parent->deleteNamedChildren(lastNode);
    } else if (leadingPath.isEmpty()) {
        this->deleteNamedChildren(lastNode);
    }
}

const VString& VSettingsNode::getName() const {
    return mName;
}

VString VSettingsNode::getPath() const {
    if (mParent == NULL) {
        return mName;
    }

    VString path = mParent->getPath();
    path += kPathDelimiterChar;
    path += mName;
    return path;
}

bool VSettingsNode::isNamed(const VString& name) const {
    return mName == name;
}

int VSettingsNode::getInt(const VString& path, int defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getIntValue();
    else
        return defaultValue;
}

int VSettingsNode::getInt(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getIntValue();

    this->throwNotFound("Integer", path);

    return 0; // (will never reach this statement because of throw)
}

int VSettingsNode::getIntValue() const {
    return static_cast<int>(this->getS64Value());
}

Vs64 VSettingsNode::getS64(const VString& path, Vs64 defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getS64Value();
    else
        return defaultValue;
}

Vs64 VSettingsNode::getS64(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getS64Value();

    this->throwNotFound("Integer", path);

    return 0; // (will never reach this statement because of throw)
}

bool VSettingsNode::getBoolean(const VString& path, bool defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getBooleanValue();
    else
        return defaultValue;
}

bool VSettingsNode::getBoolean(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getBooleanValue();

    this->throwNotFound("Boolean", path);

	return false; // (will never reach this statement because of throw)
}

VString VSettingsNode::getString(const VString& path, const VString& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getStringValue();
    else
        return defaultValue;
}

VString VSettingsNode::getString(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getStringValue();
    else
        this->throwNotFound("String", path);

   return VString::EMPTY(); // (will never reach this statement because of throw)
}

VDouble VSettingsNode::getDouble(const VString& path, VDouble defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getDoubleValue();
    else
        return defaultValue;
}

VDouble VSettingsNode::getDouble(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getDoubleValue();

    this->throwNotFound("Double", path);

    return 0.0; // (will never reach this statement because of throw)
}

VSize VSettingsNode::getSize(const VString& path, const VSize& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getSizeValue();
    else
        return defaultValue;
}

VSize VSettingsNode::getSize(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getSizeValue();

    this->throwNotFound("Size", path);

    return VSize(); // (will never reach this statement because of throw)
}

VPoint VSettingsNode::getPoint(const VString& path, const VPoint& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getPointValue();
    else
        return defaultValue;
}

VPoint VSettingsNode::getPoint(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getPointValue();

    this->throwNotFound("Point", path);

    return VPoint(); // (will never reach this statement because of throw)
}

VRect VSettingsNode::getRect(const VString& path, const VRect& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getRectValue();
    else
        return defaultValue;
}

VRect VSettingsNode::getRect(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getRectValue();

    this->throwNotFound("Rect", path);

    return VRect(); // (will never reach this statement because of throw)
}

VPolygon VSettingsNode::getPolygon(const VString& path, const VPolygon& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getPolygonValue();
    else
        return defaultValue;
}

VPolygon VSettingsNode::getPolygon(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getPolygonValue();

    this->throwNotFound("Polygon", path);

    return VPolygon(); // (will never reach this statement because of throw)
}

VColor VSettingsNode::getColor(const VString& path, const VColor& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getColorValue();
    else
        return defaultValue;
}

VColor VSettingsNode::getColor(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getColorValue();

    this->throwNotFound("Color", path);

    return VColor(); // (will never reach this statement because of throw)
}

VDuration VSettingsNode::getDuration(const VString& path, const VDuration& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getDurationValue();
    else
        return defaultValue;
}

VDuration VSettingsNode::getDuration(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getDurationValue();

    this->throwNotFound("Duration", path);

    return VDuration(); // (will never reach this statement because of throw)
}

VDate VSettingsNode::getDate(const VString& path, const VDate& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getDateValue();
    else
        return defaultValue;
}

VDate VSettingsNode::getDate(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getDateValue();

    this->throwNotFound("Date", path);

    return VDate(); // (will never reach this statement because of throw)
}

VInstant VSettingsNode::getInstant(const VString& path, const VInstant& defaultValue) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getInstantValue();
    else
        return defaultValue;
}

VInstant VSettingsNode::getInstant(const VString& path) const {
    const VSettingsNode* nodeForPath = this->findNode(path);

    if (nodeForPath != NULL)
        return nodeForPath->getInstantValue();

    this->throwNotFound("Instant", path);

    return VInstant(); // (will never reach this statement because of throw)
}

bool VSettingsNode::nodeExists(const VString& path) const {
    return (this->findNode(path) != NULL);
}

void VSettingsNode::addIntValue(const VString& path, int value) {
    this->addStringValue(path, VSTRING_INT(value));
}

void VSettingsNode::addS64Value(const VString& path, Vs64 value) {
    this->addStringValue(path, VSTRING_S64(value));
}

void VSettingsNode::addBooleanValue(const VString& path, bool value) {
    this->addStringValue(path, VSTRING_BOOL(value));
}

void VSettingsNode::addStringValue(const VString& path, const VString& value) {
    this->add(path, true, value);
}

void VSettingsNode::addDoubleValue(const VString& path, VDouble value) {
    this->addStringValue(path, VSTRING_DOUBLE(value));
}

void VSettingsNode::addSizeValue(const VString& path, const VSize& value) {
    this->addDoubleValue(path + "/width", value.getWidth());
    this->addDoubleValue(path + "/height", value.getHeight());
}

void VSettingsNode::addPointValue(const VString& path, const VPoint& value) {
    this->addDoubleValue(path + "/x", value.getX());
    this->addDoubleValue(path + "/y", value.getY());
}

void VSettingsNode::addRectValue(const VString& path, const VRect& value) {
    this->addDoubleValue(path + "/position/x", value.getLeft());
    this->addDoubleValue(path + "/position/y", value.getTop());
    this->addDoubleValue(path + "/size/width", value.getWidth());
    this->addDoubleValue(path + "/size/height", value.getHeight());
}

void VSettingsNode::addPolygonValue(const VString& path, const VPolygon& value) {
    this->add(path + "/dummy-sub1/sub2", false, VString(/*dummy*/)); // creates polygon node, need to add points to it next
    this->deleteNode(path + "/dummy-sub1"); // dummy-sub1/sub2 hack to workaround API limitation of creating desired hierarchy
    VSettingsNode* polygonNode = this->findMutableNode(path);

    const VPointVector& points = value.getPoints();
    for (VPointVector::const_iterator i = points.begin(); i != points.end(); ++i) {
        VSettingsTag* pointNode = new VSettingsTag(static_cast<VSettingsTag*>(polygonNode), "point");
        polygonNode->addChildNode(pointNode);
        pointNode->addDoubleValue("x", (*i).getX());
        pointNode->addDoubleValue("y", (*i).getY());
    }
}

void VSettingsNode::addColorValue(const VString& path, const VColor& value) {
    VString valueString(VSTRING_ARGS("#%02x%02x%02x", (Vu8)value.getRed(), (Vu8)value.getGreen(), (Vu8)value.getBlue()));
    this->addStringValue(path, valueString);
}

void VSettingsNode::addInstantValue(const VString& path, const VInstant& value, int format) {
    VString valueString;
    switch (format) {
        case VSettingsCDATA::UTC_OFFSET:
            valueString = VSTRING_S64(value.getValue());
            break;
        case VSettingsCDATA::UTC_STRING:
            valueString = value.getUTCString();
            break;
        case VSettingsCDATA::LOCAL_STRING:
            valueString = value.getLocalString();
            break;
        default:
            valueString = VSTRING_S64(value.getValue());
            break;
    }
    
    this->addStringValue(path, valueString);
}

void VSettingsNode::addDateValue(const VString& path, const VDate& value) {
    // Don't rely on default date formatter, which might change in the future. This format is what we parse.
    this->addStringValue(path, value.getDateString(VInstantFormatter("y-MM-dd")));
}

void VSettingsNode::addDurationValue(const VString& path, const VDuration& value) {
    VString valueString(VSTRING_ARGS(VSTRING_FORMATTER_S64 "ms", value.getDurationMilliseconds()));
    this->addStringValue(path, valueString);
}

void VSettingsNode::addItem(const VString& path) {
    VString dummy;
    this->add(path, false, dummy);
}

void VSettingsNode::setIntValue(const VString& path, int value) {
    this->setStringValue(path, VSTRING_INT(value));
}

void VSettingsNode::setBooleanValue(const VString& path, bool value) {
    this->setStringValue(path, VSTRING_BOOL(value));
}

void VSettingsNode::setStringValue(const VString& path, const VString& value) {
    VSettingsNode* node = this->findMutableNode(path);

    if (node == NULL)
        this->addStringValue(path, value);
    else
        node->setLiteral(value);
}

void VSettingsNode::setDoubleValue(const VString& path, VDouble value) {
    this->setStringValue(path, VSTRING_DOUBLE(value));
}

void VSettingsNode::setSizeValue(const VString& path, const VSize& value) {
    this->deleteNode(path);
    this->addSizeValue(path, value);
}

void VSettingsNode::setPointValue(const VString& path, const VPoint& value) {
    this->deleteNode(path);
    this->addPointValue(path, value);
}

void VSettingsNode::setRectValue(const VString& path, const VRect& value) {
    this->deleteNode(path);
    this->addRectValue(path, value);
}

void VSettingsNode::setPolygonValue(const VString& path, const VPolygon& value) {
    this->deleteNode(path);
    this->addPolygonValue(path, value);
}

void VSettingsNode::setColorValue(const VString& path, const VColor& value) {
    this->setStringValue(path, value.getCSSColor());
}

void VSettingsNode::setDurationValue(const VString& path, const VDuration& value) {
    VString valueString(VSTRING_ARGS(VSTRING_FORMATTER_S64 "ms", value.getDurationMilliseconds()));
    this->setStringValue(path, valueString);
}

void VSettingsNode::add(const VString& path, bool hasValue, const VString& value) {
    VString nextNodeName;
    VString theRemainder;

    VSettings::splitPathFirst(path, nextNodeName, theRemainder);

    /*
    path = a.b: next=a, rem=b -> add child a, add b to it
    path =
    */

    if (theRemainder.isEmpty()) {
        this->_addLeafValue(nextNodeName, hasValue, value);
    } else {
        VSettingsTag* child = this->_findChildTag(nextNodeName);
        if (child == NULL) {
            // If there's an attribute, need to move it down as a child tag.
            VSettingsAttribute* attribute = this->_findAttribute(nextNodeName);
            if (attribute != NULL) {
                child = new VSettingsTag(dynamic_cast<VSettingsTag*>(this), nextNodeName);
                this->addChildNode(child);

                child->addChildNode(new VSettingsCDATA(dynamic_cast<VSettingsTag*>(this), attribute->getStringValue()));

                this->_removeAttribute(attribute);
                delete attribute;
            }
        }

        if (child == NULL) {
            VString tagName(nextNodeName);

            if (nextNodeName.endsWith(']')) {
                int leftBracketIndex = nextNodeName.indexOf('[');
                nextNodeName.getSubstring(tagName, 0, leftBracketIndex);
            }

            child = new VSettingsTag(dynamic_cast<VSettingsTag*>(this), tagName);
            this->addChildNode(child);
        }

        child->add(theRemainder, hasValue, value);
    }
}

void VSettingsNode::addValue(const VString& path) {
    throw VStackTraceException(VSTRING_FORMAT("VSettingsNode::addValue called for invalid object at '%s'", path.chars()));
}

void VSettingsNode::addChildNode(VSettingsNode* /*node*/) {
    throw VStackTraceException(VSTRING_FORMAT("VSettingsNode::addChildNode called for invalid object at '%s'", this->getPath().chars()));
}

VSettingsTag* VSettingsNode::addNewChildTag(VSettingsTag* node) {
    this->addChildNode(node);
    return node;
}

VSettingsTag* VSettingsNode::getParent() {
    return mParent;
}

void VSettingsNode::_addLeafValue(const VString& name, bool /*hasValue*/, const VString& value) {
    throw VStackTraceException(VSTRING_FORMAT("VSettingsNode::_addLeafValue (%s, %s) called for invalid object at '%s'", name.chars(), value.chars(), this->getPath().chars()));
}

void VSettingsNode::throwNotFound(const VString& dataKind, const VString& missingTrail) const {
    throw VException(VSTRING_FORMAT("%s setting '%s' not found starting at path '%s'.", dataKind.chars(), missingTrail.chars(), this->getPath().chars()));
}

const char VSettingsNode::kPathDelimiterChar = '/';

// VSettings ----------------------------------------------------------------------

VSettings::VSettings() :
    VSettingsNode(NULL, VString::EMPTY()),
    mNodes() { // -> empty
}

VSettings::VSettings(const VFSNode& file) :
    VSettingsNode(NULL, VString::EMPTY()),
    mNodes() { // -> empty
    this->readFromFile(file);
}

VSettings::VSettings(VTextIOStream& inputStream) :
    VSettingsNode(NULL, VString::EMPTY()),
    mNodes() { // -> empty
    this->readFromStream(inputStream);
}

VSettings::~VSettings() {
    for (VSizeType i = 0; i < mNodes.size(); ++i)
        delete mNodes[i];
}

void VSettings::readFromFile(const VFSNode& file) {
    VBufferedFileStream fs(file);
    fs.openReadOnly();
    VTextIOStream in(fs);
    this->readFromStream(in);
}

void VSettings::writeToFile(const VFSNode& file) const {
    VFileWriter writer(file);
    this->writeToStream(writer.getTextOutputStream());
    writer.save();
}

void VSettings::readFromStream(VTextIOStream& inputStream) {
    vault::vectorDeleteAll(mNodes);

    VSettingsXMLParser parser(inputStream, &mNodes);

    parser.parse();
}

void VSettings::writeToStream(VTextIOStream& outputStream, int indentLevel) const {
    for (VSizeType i = 0; i < mNodes.size(); ++i) {
        mNodes[i]->writeToStream(outputStream, indentLevel);
    }
}

VBentoNode* VSettings::writeToBento() const {
    VBentoNode* topNode = new VBentoNode();

    for (VSizeType i = 0; i < mNodes.size(); ++i) {
        VBentoNode* theNode = mNodes[i]->writeToBento();
        topNode->addChildNode(theNode);
    }

    return topNode;
}

void VSettings::debugPrint() const {
    VMemoryStream   memoryStream;
    VTextIOStream   outputStream(memoryStream);

    this->writeToStream(outputStream);

    std::cout << "Begin Settings:\n";

    // Avoid stdout flush problems: print buffer one line at a time.
    char*       buffer = reinterpret_cast<char*>(memoryStream.getBuffer());
    VSizeType   lengthRemaining = strlen(buffer);
    VString     s;

    while (lengthRemaining > 0) {
        s = VString::EMPTY();

        char c = *buffer;
        ++buffer;
        --lengthRemaining;

        while ((c != '\n') && (c != '\r') && (lengthRemaining > 0)) {
            s += c;

            c = *buffer;
            ++buffer;
            --lengthRemaining;
        }

        std::cout << s.chars() << '\n';
        fflush(stdout);
    }

    std::cout << "End Settings\n";

    fflush(stdout);
}

const VSettingsNode* VSettings::findNode(const VString& path)  const {
    VString nextNodeName;
    VString theRemainder;

    VSettings::splitPathFirst(path, nextNodeName, theRemainder);

    VSettingsTag* child = this->_findChildTag(nextNodeName);
    if (child != NULL)
        return child->findNode(theRemainder);
    else
        return NULL;
}

int VSettings::countNamedChildren(const VString& name) const {
    int     result = 0;

    for (VSizeType i = 0; i < mNodes.size(); ++i) {
        if (mNodes[i]->getName() == name) {
            ++result;
        }
    }

    return result;
}

const VSettingsNode* VSettings::getNamedChild(const VString& name, int index) const {
    int     numFound = 0;

    for (VSizeType i = 0; i < mNodes.size(); ++i) {
        VSettingsNode* child = mNodes[i];

        if (child->getName() == name) {
            if (numFound == index) {
                return child;
            }

            ++numFound;
        }
    }

    return NULL;
}

void VSettings::deleteNamedChildren(const VString& name) {
    // Iterate backwards so it's safe to delete while iterating.

    for (VSizeType i = mNodes.size(); i > 0 ; --i) {
        VSettingsNode* child = mNodes[i-1];

        if (child->getName() == name) {
            delete child;
            mNodes.erase(mNodes.begin() + i - 1);
        }
    }
}

Vs64 VSettings::getS64Value() const {
    throw VStackTraceException("Tried to get raw int value on top level settings object.");
}

bool VSettings::getBooleanValue() const {
    throw VStackTraceException("Tried to get raw boolean value on top level settings object.");
}

VString VSettings::getStringValue() const {
    throw VStackTraceException("Tried to get raw string value on top level settings object.");
}

VSize VSettings::getSizeValue() const {
    throw VStackTraceException("Tried to get raw size value on top level settings object.");
}

VDouble VSettings::getDoubleValue() const {
    throw VStackTraceException("Tried to get raw double value on top level settings object.");
}

VPoint VSettings::getPointValue() const {
    throw VStackTraceException("Tried to get raw point value on top level settings object.");
}

VRect VSettings::getRectValue() const {
    throw VStackTraceException("Tried to get raw rect value on top level settings object.");
}

VPolygon VSettings::getPolygonValue() const {
    throw VStackTraceException("Tried to get raw polygon value on top level settings object.");
}

VColor VSettings::getColorValue() const {
    throw VStackTraceException("Tried to get raw color value on top level settings object.");
}

VDuration VSettings::getDurationValue() const {
    throw VStackTraceException("Tried to get raw duration value on top level settings object.");
}

VDate VSettings::getDateValue() const {
    throw VStackTraceException("Tried to get raw date value on top level settings object.");
}

VInstant VSettings::getInstantValue() const {
    throw VStackTraceException("Tried to get raw instant value on top level settings object.");
}

void VSettings::addChildNode(VSettingsNode* node) {
    mNodes.push_back(node);
}

// static
bool VSettings::stringToBoolean(const VString& value) {
    return (value == "1" ||
            value == "T" ||
            value == "t" ||
            value == "Y" ||
            value == "y" ||
            value == "TRUE" ||
            value == "true" ||
            value == "YES" ||
            value == "yes");
}

// static
bool VSettings::isPathLeaf(const VString& path) {
    return !path.contains(kPathDelimiterChar);
}

// static
void VSettings::splitPathFirst(const VString& path, VString& nextNodeName, VString& outRemainder) {
    // This code handles a leaf even though we kind of expect callers to check that first.

    int dotLocation = path.indexOf(kPathDelimiterChar);

    path.getSubstring(nextNodeName, 0, dotLocation);

    if (dotLocation == -1)    // no dot found
        outRemainder = VString::EMPTY();
    else
        path.getSubstring(outRemainder, dotLocation + 1);
}

// static
void VSettings::splitPathLast(const VString& path, VString& leadingPath, VString& lastNode) {
    // This code handles a leaf even though we kind of expect callers to check that first.

    int dotLocation = path.lastIndexOf(kPathDelimiterChar);

    if (dotLocation == -1)    // no dot found
        leadingPath = VString::EMPTY();
    else
        path.getSubstring(leadingPath, 0, dotLocation);

    path.getSubstring(lastNode, dotLocation + 1);
}

VSettingsTag* VSettings::_findChildTag(const VString& name) const {
    for (VSizeType i = 0; i < mNodes.size(); ++i) {
        if (mNodes[i]->isNamed(name)) {
            return static_cast<VSettingsTag*>(mNodes[i]);
        }
    }

    return NULL;
}

void VSettings::_addLeafValue(const VString& name, bool /*hasValue*/, const VString& value) {
    VString tagName(name);

    if (name.endsWith(']')) {
        int leftBracketIndex = name.indexOf('[');
        name.getSubstring(tagName, 0, leftBracketIndex);
    }

    VSettingsTag* tag = new VSettingsTag(NULL, tagName);

    tag->addChildNode(new VSettingsCDATA(tag, value));

    mNodes.push_back(tag);
}

// VSettingsTag ------------------------------------------------------------------

VSettingsTag::VSettingsTag(VSettingsTag* parent, const VString& name) :
    VSettingsNode(parent, name),
    mAttributes(), // -> empty
    mChildNodes() { // -> empty
}

VSettingsTag::~VSettingsTag() {
    for (VSizeType i = 0; i < mAttributes.size(); ++i)
        delete mAttributes[i];

    for (VSizeType i = 0; i < mChildNodes.size(); ++i)
        delete mChildNodes[i];
}

void VSettingsTag::writeToStream(VTextIOStream& outputStream, int indentLevel) const {
    for (int i = 0; i < indentLevel; ++i)
        outputStream.writeString(_INDENT_STRING);

    VString beginTag(VSTRING_ARGS("<%s", mName.chars()));
    outputStream.writeString(beginTag);

    if (!mAttributes.empty()) {
        // Write each attribute
        for (VSizeType i = 0; i < mAttributes.size(); ++i) {
            outputStream.writeString(" ");
            mAttributes[i]->writeToStream(outputStream);
        }
    }

    if (mChildNodes.empty()) {
        // Just close the tag and we're done.
        outputStream.writeLine(" />");
    } else if ((mChildNodes.size() == 1) && (dynamic_cast<VSettingsCDATA*>(mChildNodes[0]) != nullptr)) {
        // The tag has only a CDATA child, so render the tag and contents on one line.
        outputStream.writeString(">");
        outputStream.writeString(mChildNodes[0]->getStringValue());
        VString endTag(VSTRING_ARGS("</%s>", mName.chars()));
        outputStream.writeLine(endTag);
    } else {
        // Close the opening tag.
        outputStream.writeLine(">");

        // Write each child node
        for (VSizeType i = 0; i < mChildNodes.size(); ++i) {
            mChildNodes[i]->writeToStream(outputStream, indentLevel + 1);
        }

        // Write a closing tag.
        for (int i = 0; i < indentLevel; ++i) {
            outputStream.writeString(_INDENT_STRING);
        }

        VString endTag(VSTRING_ARGS("</%s>", mName.chars()));
        outputStream.writeLine(endTag);
    }

}

VBentoNode* VSettingsTag::writeToBento() const {
    VBentoNode* tagNode = new VBentoNode(mName);

    if (!mAttributes.empty()) {
        // Write each attribute
        for (VSizeType i = 0; i < mAttributes.size(); ++i) {
            tagNode->addString(mAttributes[i]->getName(), mAttributes[i]->getStringValue());
        }
    }

    if (!mChildNodes.empty()) {
        // Write each child node
        for (VSizeType i = 0; i < mChildNodes.size(); ++i) {
            VBentoNode* childNode = mChildNodes[i]->writeToBento();
            tagNode->addChildNode(childNode);
        }
    }

    return tagNode;
}

int VSettingsTag::countNamedChildren(const VString& name) const {
    int     result = 0;

    for (VSizeType i = 0; i < mAttributes.size(); ++i) {
        if (mAttributes[i]->getName() == name) {
            ++result;
        }
    }

    for (VSizeType i = 0; i < mChildNodes.size(); ++i) {
        if (mChildNodes[i]->getName() == name) {
            ++result;
        }
    }

    return result;
}

const VSettingsNode* VSettingsTag::getNamedChild(const VString& name, int index) const {
    int     numFound = 0;

    for (VSizeType i = 0; i < mAttributes.size(); ++i) {
        VSettingsAttribute* attribute = mAttributes[i];

        if (attribute->getName() == name) {
            if (numFound == index) {
                return attribute;
            }

            ++numFound;
        }
    }

    for (VSizeType i = 0; i < mChildNodes.size(); ++i) {
        VSettingsNode* child = mChildNodes[i];

        if (child->getName() == name) {
            if (numFound == index) {
                return child;
            }

            ++numFound;
        }
    }

    return NULL;
}

void VSettingsTag::deleteNamedChildren(const VString& name) {
    // Iterate backwards so it's safe to delete while iterating.

    for (VSizeType i = mAttributes.size(); i > 0; --i) {
        VSettingsAttribute* attribute = mAttributes[i-1];

        if (attribute->getName() == name) {
            delete attribute;
            mAttributes.erase(mAttributes.begin() + i - 1);
        }
    }

    for (VSizeType i = mChildNodes.size(); i > 0 ; --i) {
        VSettingsNode* child = mChildNodes[i-1];

        if (child->getName() == name) {
            delete child;
            mChildNodes.erase(mChildNodes.begin() + i - 1);
        }
    }
}

void VSettingsTag::addAttribute(VSettingsAttribute* attribute) {
    mAttributes.push_back(attribute);
}

void VSettingsTag::addChildNode(VSettingsNode* node) {
    mChildNodes.push_back(node);
}

Vs64 VSettingsTag::getS64Value() const {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        this->throwNotFound("Integer", "<cdata>");

    return cdataNode->getS64Value();
}

bool VSettingsTag::getBooleanValue() const {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        this->throwNotFound("Boolean", "<cdata>");

    return cdataNode->getBooleanValue();
}

VString VSettingsTag::getStringValue() const {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        return VString::EMPTY(); // unlike other data types, an empty string in an explicit attribute is a legitimate "value"
    else
        return cdataNode->getStringValue();
}

VDouble VSettingsTag::getDoubleValue() const {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        this->throwNotFound("Double", "<cdata>");

    return cdataNode->getDoubleValue();
}

VSize VSettingsTag::getSizeValue() const {
    VSize size(this->getDouble("width"), this->getDouble("height"));
    return size;
}

VPoint VSettingsTag::getPointValue() const {
    VPoint point(this->getDouble("x"), this->getDouble("y"));
    return point;
}

VRect VSettingsTag::getRectValue() const {
    VRect rect(this->getDouble("position/x"), this->getDouble("position/y"),
               this->getDouble("size/width"), this->getDouble("size/height"));
    return rect;
}

VPolygon VSettingsTag::getPolygonValue() const {
    VPolygon polygon;
    int numPoints = this->countNamedChildren("point");
    for (int i = 0; i < numPoints; ++i) {
        const VSettingsTag* pointTag = static_cast<const VSettingsTag*>(this->getNamedChild("point", i));
        polygon.add(pointTag->getPointValue());
    }

    return polygon;
}

VColor VSettingsTag::getColorValue() const {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        this->throwNotFound("Color", "<cdata>");

    return cdataNode->getColorValue();
}

VDuration VSettingsTag::getDurationValue() const {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        this->throwNotFound("Duration", "<cdata>");

    return cdataNode->getDurationValue();
}

VDate VSettingsTag::getDateValue() const {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        this->throwNotFound("Date", "<cdata>");

    return cdataNode->getDateValue();
}

VInstant VSettingsTag::getInstantValue() const {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        this->throwNotFound("Instant", "<cdata>");

    return cdataNode->getInstantValue();
}

void VSettingsTag::setLiteral(const VString& value) {
    VSettingsNode* cdataNode = this->_findChildTag("<cdata>");

    if (cdataNode == NULL)
        this->throwNotFound("String", "<cdata>");

    cdataNode->setLiteral(value);
}

VSettingsAttribute* VSettingsTag::_findAttribute(const VString& name) const {
    for (VSizeType i = 0; i < mAttributes.size(); ++i) {
        if (mAttributes[i]->isNamed(name)) {
            return mAttributes[i];
        }
    }

    return NULL;
}

VSettingsTag* VSettingsTag::_findChildTag(const VString& name) const {
    if (name.endsWith(']')) {
        int     leftBracketIndex = name.indexOf('[');
        VString indexString;
        name.getSubstring(indexString, leftBracketIndex + 1, name.length() - 1);
        int     theIndex = indexString.parseInt();

        VString nameOnly;
        name.getSubstring(nameOnly, 0, leftBracketIndex);

        return static_cast<VSettingsTag*>(const_cast<VSettingsNode*>(this->getNamedChild(nameOnly, theIndex))); // const_cast: NON-CONST RETURN
    } else {
        for (VSizeType i = 0; i < mChildNodes.size(); ++i) {
            if (mChildNodes[i]->isNamed(name)) {
                return static_cast<VSettingsTag*>(mChildNodes[i]);
            }
        }
    }

    return NULL;
}

void VSettingsTag::_addLeafValue(const VString& name, bool hasValue, const VString& value) {
    if (hasValue) {
        if (mPreferCDATA) {
            VSettingsTag* tag = new VSettingsTag(NULL, name);
            tag->addChildNode(new VSettingsCDATA(tag, value));
            this->addChildNode(tag);
        } else {
            this->addAttribute(new VSettingsAttribute(this, name, value));
        }
    } else {
        this->addAttribute(new VSettingsAttribute(this, name));
    }
}

void VSettingsTag::_removeAttribute(VSettingsAttribute* attribute) {
    for (VSettingsAttributePtrVector::iterator i = mAttributes.begin(); i != mAttributes.end(); ++i) {
        if ((*i) == attribute) {
            mAttributes.erase(i);
            return;
        }
    }
}

void VSettingsTag::_removeChildNode(VSettingsNode* child) {
    for (VSettingsNodePtrVector::iterator i = mChildNodes.begin(); i != mChildNodes.end(); ++i) {
        if ((*i) == child) {
            mChildNodes.erase(i);
            return;
        }
    }
}

// VSettingsAttribute ------------------------------------------------------------

VSettingsAttribute::VSettingsAttribute(VSettingsTag* parent, const VString& name, const VString& value) :
    VSettingsNode(parent, name),
    mHasValue(true),
    mValue(value) {
}

VSettingsAttribute::VSettingsAttribute(VSettingsTag* parent, const VString& name) :
    VSettingsNode(parent, name),
    mHasValue(false),
    mValue() { // -> empty string
}

void VSettingsAttribute::writeToStream(VTextIOStream& outputStream, int /*indentLevel*/) const {
    if (mHasValue) {
        VString attributeString(VSTRING_ARGS("%s=\"%s\"", mName.chars(), mValue.chars()));
        outputStream.writeString(attributeString);
    } else {
        VString attributeString(VSTRING_ARGS("%s", mName.chars()));
        outputStream.writeString(attributeString);
    }
}

VBentoNode* VSettingsAttribute::writeToBento() const {
    return NULL;        // This doesn't create a node
}

Vs64 VSettingsAttribute::getS64Value() const {
    return mValue.parseS64();
}

bool VSettingsAttribute::getBooleanValue() const {
    return VSettings::stringToBoolean(mValue);
}

VString VSettingsAttribute::getStringValue() const {
    return mValue;
}

VDouble VSettingsAttribute::getDoubleValue() const {
    return mValue.parseDouble();
}

VSize VSettingsAttribute::getSizeValue() const {
    this->throwNotFound("Size", "attribute");
    return VSize(); // (will never reach this statement because of throw)
}

VPoint VSettingsAttribute::getPointValue() const {
    this->throwNotFound("Point", "attribute");
    return VPoint(); // (will never reach this statement because of throw)
}

VRect VSettingsAttribute::getRectValue() const {
    this->throwNotFound("Rect", "attribute");
    return VRect(); // (will never reach this statement because of throw)
}

VPolygon VSettingsAttribute::getPolygonValue() const {
    this->throwNotFound("Polygon", "attribute");
    return VPolygon(); // (will never reach this statement because of throw)
}

VColor VSettingsAttribute::getColorValue() const {
    return VColor(mValue);
}

VDuration VSettingsAttribute::getDurationValue() const {
    return VDuration::createFromDurationString(mValue);
}

VDate VSettingsAttribute::getDateValue() const {
    return VDate::createFromDateString(mValue, VCodePoint('-'));
}

VInstant VSettingsAttribute::getInstantValue() const {
    VInstant when;
    if (mValue.contains("UTC")) {
        when.setUTCString(mValue);
    } else {
        bool isNumeric = true;
        for (VString::const_iterator i = mValue.begin(); i != mValue.end(); ++i) {
            if (! (*i).isNumeric()) {
                isNumeric = false;
                break;
            }
        }
    
        if (isNumeric) {
            when = VInstant::instantFromRawValue(mValue.parseS64());
        } else {
            when.setLocalString(mValue);
        }
    }
    return when;
}

void VSettingsAttribute::setLiteral(const VString& value) {
    mHasValue = true;
    mValue = value;
}

bool VSettingsAttribute::hasValue() const {
    return mHasValue;
}

// VSettingsCDATA ------------------------------------------------------------

VSettingsCDATA::VSettingsCDATA(VSettingsTag* parent, const VString& cdata) :
    VSettingsNode(parent, "<cdata>"),
    mCDATA(cdata) {
}

void VSettingsCDATA::writeToStream(VTextIOStream& outputStream, int indentLevel) const {
    if (indentLevel > 1) {  // at indent level 1 we're just a top-level item, indenting is detrimental
        for (int i = 0; i < indentLevel; ++i) {
            outputStream.writeString(_INDENT_STRING);
        }
    }

    outputStream.writeLine(mCDATA);
}

VBentoNode* VSettingsCDATA::writeToBento() const {
    VBentoNode* cDataNode = new VBentoNode(mName);
    cDataNode->addString(mName, mCDATA);
    return cDataNode;
}

Vs64 VSettingsCDATA::getS64Value() const {
    return mCDATA.parseS64();
}

bool VSettingsCDATA::getBooleanValue() const {
    return VSettings::stringToBoolean(mCDATA);
}

VString VSettingsCDATA::getStringValue() const {
    return mCDATA;
}

VDouble VSettingsCDATA::getDoubleValue() const {
    return mCDATA.parseDouble();
}

VSize VSettingsCDATA::getSizeValue() const {
    this->throwNotFound("Size", "attribute");
    return VSize(); // (will never reach this statement because of throw)
}

VPoint VSettingsCDATA::getPointValue() const {
    this->throwNotFound("Point", "attribute");
    return VPoint(); // (will never reach this statement because of throw)
}

VRect VSettingsCDATA::getRectValue() const {
    this->throwNotFound("Rect", "attribute");
    return VRect(); // (will never reach this statement because of throw)
}

VPolygon VSettingsCDATA::getPolygonValue() const {
    this->throwNotFound("Polygon", "attribute");
    return VPolygon(); // (will never reach this statement because of throw)
}

VColor VSettingsCDATA::getColorValue() const {
    return VColor(mCDATA);
}

VDuration VSettingsCDATA::getDurationValue() const {
    return VDuration::createFromDurationString(mCDATA);
}

VDate VSettingsCDATA::getDateValue() const {
    return VDate::createFromDateString(mCDATA, VCodePoint('-'));
}

VInstant VSettingsCDATA::getInstantValue() const {
    VInstant when;
    if (mCDATA.contains("UTC")) {
        when.setUTCString(mCDATA);
    } else {
        when.setLocalString(mCDATA);
    }
    return when;
}

void VSettingsCDATA::setLiteral(const VString& value) {
    mCDATA = value;
}

// VSettingsXMLParser --------------------------------------------------------

VSettingsXMLParser::VSettingsXMLParser(VTextIOStream& inputStream, VSettingsNodePtrVector* nodes) :
    mInputStream(inputStream),
    mNodes(nodes),
    mCurrentLine(), // -> empty string
    mCurrentLineNumber(0),
    mCurrentColumnNumber(0),
    mParserState(kReady),
    mElement(), // -> empty string
    mCurrentTag(NULL),
    mPendingAttributeName() { // -> empty string
}

void VSettingsXMLParser::parse() {
    bool    done = false;
    VString line;

    mParserState = kReady;

    while (! done) {
        try {
            mInputStream.readLine(mCurrentLine);

            ++mCurrentLineNumber;

            this->parseLine();
        } catch (const VEOFException& /*ex*/) {
            done = true;
        }
    }
}

void VSettingsXMLParser::parseLine() {

    mCurrentColumnNumber = 0;

    if ((mCurrentLineNumber == 1) && mCurrentLine.startsWith("<?") && mCurrentLine.endsWith("?>")) {
        return; // skip the typical "<?xml version .... ?>" first line
    }

    for (VString::iterator i = mCurrentLine.begin(); i != mCurrentLine.end(); ++i) {
        VCodePoint c = (*i);

        ++mCurrentColumnNumber;

        switch (mParserState) {
            case kReady:
                if (c == '<') {
                    this->emitCDATA();
                    this->changeState(kTag1_open);
                } else {
                    this->accumulate(c);
                }
                break;

            case kComment1_bang:
                if (c == '-') {
                    this->changeState(kComment2_bang_dash);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' after presumed start of comment.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kComment2_bang_dash:
                if (c == '-') {
                    this->changeState(kComment3_in_comment);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%c' after presumed start of comment.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kComment3_in_comment:
                if (c == '-') {
                    this->changeState(kComment4_traildash);
                } else {
                    /*nothing*/
                }
                break;

            case kComment4_traildash:
                if (c == '-') {
                    this->changeState(kComment5_traildash_dash);
                } else {
                    this->changeState(kComment3_in_comment);
                }
                break;

            case kComment5_traildash_dash:
                if (c == '-') {
                    // *nothing
                } else if (c == '>') {
                    this->changeState(kReady);
                } else {
                    this->changeState(kComment3_in_comment);
                }
                break;

            case kTag1_open:
                if (c == '!') {
                    this->changeState(kComment1_bang);
                } else if (c == '/') {
                    this->changeState(kCloseTag1_open_slash);
                } else if (c.isAlpha()) {
                    this->changeState(kTag2_in_name);
                    this->accumulate(c);
                } else if (c.isWhitespace()) {
                    // nothing
                } else {
                    this->stateError("Invalid character after opening tag bracket.");
                }
                break;

            case kTag2_in_name:
                if (VSettingsXMLParser::isValidTagNameChar(c)) {
                    this->accumulate(c);
                } else if (c.isWhitespace()) {
                    this->emitOpenTagName();
                    this->changeState(kTag3_post_name);
                } else if (c == '/') {
                    this->emitOpenTagName();
                    this->changeState(kTag8_solo_close_slash);
                } else if (c == '>') {
                    this->emitOpenTagName();
                    this->changeState(kReady);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' in tag name.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kTag3_post_name:
                if (c.isWhitespace()) {
                    // nothing
                } else if (c == '>') {
                    this->changeState(kReady);
                } else if (c == '/') {
                    this->changeState(kTag8_solo_close_slash);
                } else if (c.isAlpha()) {
                    this->changeState(kTag4_in_attribute_name);
                    this->accumulate(c);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' in tag after name.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kTag4_in_attribute_name:
                if (VSettingsXMLParser::isValidAttributeNameChar(c)) {
                    this->accumulate(c);
                } else if (c == '=') {
                    this->emitAttributeName();
                    this->changeState(kTag5_attribute_equals);
                } else if (c.isWhitespace()) {
                    this->emitAttributeNameOnly();
                    this->changeState(kTag3_post_name);
                } else if (c == '/') {
                    this->emitAttributeNameOnly();
                    this->changeState(kTag8_solo_close_slash);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' in attribute name.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kTag5_attribute_equals:
                if (c == '\"') {
                    this->changeState(kTag6_attribute_quoted);
                } else if (c == '/') {
                    this->emitAttributeValue();
                    this->changeState(kTag8_solo_close_slash);
                } else if (c.isAlphaNumeric()) {
                    this->changeState(kTag7_attribute_unquoted);
                    this->accumulate(c);
                }
                break;

            case kTag6_attribute_quoted:
                if (c.isAlphaNumeric()) {
                    this->accumulate(c);
                } else if (c.isWhitespace()) {
                    this->accumulate(c);
                } else if (c == '\"') {
                    this->emitAttributeValue();
                    this->changeState(kTag3_post_name);
                } else {
                    this->accumulate(c);
                }
                break;

            case kTag7_attribute_unquoted:
                if (VSettingsXMLParser::isValidAttributeValueChar(c)) {
                    this->accumulate(c);
                } else if (c.isWhitespace()) {
                    this->emitAttributeValue();
                    this->changeState(kTag3_post_name);
                } else if (c == '>') {
                    this->emitAttributeValue();
                    this->changeState(kReady);
                } else if (c == '/') {
                    this->emitAttributeValue();
                    this->changeState(kTag8_solo_close_slash);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' in unquoted attribute value.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kTag8_solo_close_slash:
                if (c == '>') {
                    this->emitEndSoloTag();
                    this->changeState(kReady);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' after solo close tag slash.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kCloseTag1_open_slash:
                if (c.isWhitespace()) {
                    // nothing
                } else if (VSettingsXMLParser::isValidTagNameChar(c)) {
                    this->changeState(kCloseTag2_in_name);
                    this->accumulate(c);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' in closing tag.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kCloseTag2_in_name:
                if (c == '>') {
                    this->emitCloseTagName();
                    this->changeState(kReady);
                } else if (c.isWhitespace()) {
                    this->emitCloseTagName();
                    this->changeState(kCloseTag3_trailing_whitespace);
                } else if (VSettingsXMLParser::isValidTagNameChar(c)) {
                    this->accumulate(c);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' in closing tag.", c.toString().chars()));
                    this->stateError(s);
                }
                break;

            case kCloseTag3_trailing_whitespace:
                if (c.isWhitespace()) {
                    // nothing
                } else if (c == '>') {
                    this->changeState(kReady);
                } else {
                    VString s(VSTRING_ARGS("Invalid character '%s' in closing tag.", c.toString().chars()));
                    this->stateError(s);
                }
                break;
        }

        if (c == '\t') {
            mCurrentColumnNumber += 3;    // already did ++, and we want tabs to be 4 "columns" in terms of syntax errors
        }
    }
}

void VSettingsXMLParser::resetElement() {
    mElement = VString::EMPTY();
}

void VSettingsXMLParser::accumulate(const VCodePoint& c) {
    mElement += c;
}

void VSettingsXMLParser::changeState(ParserState newState) {
    mParserState = newState;
    this->resetElement();
}

void VSettingsXMLParser::stateError(const VString& errorMessage) {
    VString completeMessage(VSTRING_ARGS("Syntax error in state %d at line %d, column %d: %s", mParserState, mCurrentLineNumber, mCurrentColumnNumber, errorMessage.chars()));
    throw VStackTraceException(completeMessage);
}

void VSettingsXMLParser::emitCDATA() {
    mElement.trim();

    if (! mElement.isEmpty()) {
        if (mCurrentTag == NULL)
            mNodes->push_back(new VSettingsCDATA(NULL, mElement));
        else
            mCurrentTag->addChildNode(new VSettingsCDATA(mCurrentTag, mElement));
    }
}

void VSettingsXMLParser::emitOpenTagName() {
    VSettingsTag* tag = new VSettingsTag(mCurrentTag, mElement);

    if (mCurrentTag == NULL)
        mNodes->push_back(tag);
    else
        mCurrentTag->addChildNode(tag);

    mCurrentTag = tag;
}

void VSettingsXMLParser::emitAttributeName() {
    mPendingAttributeName = mElement;
}

void VSettingsXMLParser::emitAttributeNameOnly() {
    VSettingsAttribute* attribute = new VSettingsAttribute(mCurrentTag, mElement);
    mCurrentTag->addAttribute(attribute);
}

void VSettingsXMLParser::emitAttributeValue() {
    VSettingsAttribute* attribute = new VSettingsAttribute(mCurrentTag, mPendingAttributeName, mElement);
    mCurrentTag->addAttribute(attribute);
}

void VSettingsXMLParser::emitCloseTagName() {
    if (mCurrentTag->getName() != mElement)
        this->stateError(VSTRING_FORMAT("Closing tag name '%s' does not balance opening tag '%s'.", mElement.chars(), mCurrentTag->getName().chars()));

    mCurrentTag = mCurrentTag->getParent();
}

void VSettingsXMLParser::emitEndSoloTag() {
    mCurrentTag = mCurrentTag->getParent();
}

// static
bool VSettingsXMLParser::isValidTagNameChar(const VCodePoint& c) {
    int value = c.intValue();
    return ((value > 0x20) && (value < 0x7F) && (value != '<') && (value != '>') && (value != '/') && (value != '='));
}

// static
bool VSettingsXMLParser::isValidAttributeNameChar(const VCodePoint& c) {
    int value = c.intValue();
    return ((value > 0x20) && (value < 0x7F) && (value != '<') && (value != '>') && (value != '/') && (value != '='));
}

// static
bool VSettingsXMLParser::isValidAttributeValueChar(const VCodePoint& c) {
    int value = c.intValue();
    return ((value > 0x20) && (value < 0x7F) && (value != '<') && (value != '>') && (value != '/') && (value != '='));
}

