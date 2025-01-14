/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <OpenVRUI/vsg/VSGVruiTexturedBackground.h>

#include <OpenVRUI/sginterface/vruiRendererInterface.h>

#include <OpenVRUI/vsg/VSGVruiPresets.h>
#include <OpenVRUI/vsg/VSGVruiTransformNode.h>
#include <OpenVRUI/vsg/VSGVruiTexture.h>

#include <OpenVRUI/coTexturedBackground.h>

#include <vsg/nodes/MatrixTransform.h>

#include <OpenVRUI/util/vruiLog.h>

using namespace vsg;
using namespace std;

namespace vrui
{

static const int MipMapLevels = 8;

vsg::ref_ptr<vsg::vec3Array> VSGVruiTexturedBackground::normal;

/** Constructor
  @param backgroundMaterial normal color
  @param highlightMaterial highlighted color
  @param disableMaterial disabled color
  @see coUIElement for color definition
*/
VSGVruiTexturedBackground::VSGVruiTexturedBackground(coTexturedBackground *background)
    : VSGVruiUIContainer(background)
{
    this->background = background;

    state = 0;
    highlightState = 0;
    disabledState = 0;
    repeat = background->getRepeat();
    texXSize = background->getTexXSize();
    texYSize = background->getTexYSize();
}

/** Destructor
 */
VSGVruiTexturedBackground::~VSGVruiTexturedBackground()
{
}

void VSGVruiTexturedBackground::update()
{
  /*  geometryNode->dirtyBound();
    geometry->dirtyBound();
    geometry->dirtyDisplayList();
    if (tex != background->getCurrentTextures() || background->getUpdated())
    {
        background->setUpdated(false);

        tex = background->getCurrentTextures();

        Image *image = texNormal->getImage();


        GLenum pixel_format = (GLenum)tex->pixelFormat;
        if(pixel_format == coTexturedBackground::TextureSet::PF_DEFAULT)
            pixel_format = tex->comp == 3 ? GL_RGB : GL_RGBA;

        image->setImage(tex->s, tex->t, tex->r, tex->comp == 3 ? GL_RGB : GL_RGBA, pixel_format, GL_UNSIGNED_BYTE,
                        (unsigned char *)tex->normalTextureImage, Image::NO_DELETE, tex->comp);
        image->dirty();
        texNormal->dirtyTextureObject();

        ref_ptr<TexEnv> texEnv = VSGVruiPresets::getTexEnvModulate();

        state->setTextureAttributeAndModes(0, texNormal.get(), StateAttribute::ON | StateAttribute::PROTECTED);
        state->setTextureAttributeAndModes(0, texEnv.get(), StateAttribute::ON | StateAttribute::PROTECTED);

        if (tex->highlightedTextureImage)
        {
            image = texHighlighted->getImage();
            image->setImage(tex->s, tex->t, tex->r, pixel_format, pixel_format, GL_UNSIGNED_BYTE,
                            (unsigned char *)tex->highlightedTextureImage, Image::NO_DELETE, tex->comp);
            image->dirty();
            texHighlighted->dirtyTextureObject();

            highlightState->setTextureAttributeAndModes(0, texHighlighted.get(), StateAttribute::ON | StateAttribute::PROTECTED);
            highlightState->setTextureAttributeAndModes(0, texEnv.get(), StateAttribute::ON | StateAttribute::PROTECTED);
        }

        if (tex->disabledTextureImage)
        {
            image = texDisabled->getImage();
            image->setImage(tex->s, tex->t, tex->r, pixel_format, pixel_format, GL_UNSIGNED_BYTE,
                            (unsigned char *)tex->disabledTextureImage, Image::NO_DELETE, tex->comp);
            image->dirty();
            texDisabled->dirtyTextureObject();

            disabledState->setTextureAttributeAndModes(0, texHighlighted.get(), StateAttribute::ON | StateAttribute::PROTECTED);
            disabledState->setTextureAttributeAndModes(0, texEnv.get(), StateAttribute::ON | StateAttribute::PROTECTED);
        }
    }

    if (repeat != background->getRepeat())
    {

        repeat = background->getRepeat();

        if (repeat)
        {
            texNormal->setWrap(Texture::WRAP_S, Texture::REPEAT);
            texNormal->setWrap(Texture::WRAP_T, Texture::REPEAT);
            texHighlighted->setWrap(Texture::WRAP_S, Texture::REPEAT);
            texHighlighted->setWrap(Texture::WRAP_T, Texture::REPEAT);
            texDisabled->setWrap(Texture::WRAP_S, Texture::REPEAT);
            texDisabled->setWrap(Texture::WRAP_T, Texture::REPEAT);
        }
        else
        {
            texNormal->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
            texNormal->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
            texHighlighted->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
            texHighlighted->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
            texDisabled->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
            texDisabled->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
        }
    }

    //    if (texXSize != background->getTexXSize() || texYSize != background->getTexYSize())
    rescaleTexture();*/
}

void VSGVruiTexturedBackground::resizeGeometry()
{
    createGeometry();

    float myHeight = background->getHeight();
    float myWidth = background->getWidth();

   /* (*coord)[3].set(0.0f, myHeight, 0.0f);
    (*coord)[2].set(myWidth, myHeight, 0.0f);
    (*coord)[1].set(myWidth, 0.0f, 0.0f);
    (*coord)[0].set(0.0f, 0.0f, 0.0f);

    rescaleTexture();
	coord->dirty();
    geometryNode->dirtyBound();
    geometry->dirtyBound();
    geometry->dirtyDisplayList();*/
}

/// resize the texture if texture width or height is set to 0
void VSGVruiTexturedBackground::rescaleTexture()
{
    float xmin = 0.0f;
    float xmax = 1.0f;
    float ymin = 0.0f;
    float ymax = 1.0f;

    float myHeight = background->getHeight();
    float myWidth = background->getWidth();
    //fprintf(stderr, "\nmyHeight=%f  myWidth=%f\n", myHeight, myWidth);

    texXSize = background->getTexXSize();
    texYSize = background->getTexYSize();
    //fprintf(stderr, "texXSize=%f  texYSize=%f\n", texXSize, texYSize);

    float myScale = background->getScale();
    if (myScale == 0.0)
        myScale = 1.0;

    if ((myWidth > 0) && (texXSize < myWidth))
    {
        if (texXSize != 0)
        {
            xmin = ((myWidth - texXSize) / myWidth) / 2.0f;
            xmax = (((myWidth - texXSize) / myWidth) / 2.0f + texXSize / myWidth) / myScale;
        }
    }

    if ((myHeight > 0) && (texYSize < myHeight))
    {
        if (texYSize != 0)
        {
            ymin = ((myHeight - texYSize) / myHeight) / 2.0f;
            ymax = (((myHeight - texYSize) / myHeight) / 2.0f + texYSize / myHeight) / myScale;
        }
    }

    (*texcoord)[0].set(xmin, ymin);
    (*texcoord)[1].set(xmax, ymin);
    (*texcoord)[2].set(xmax, ymax);
    (*texcoord)[3].set(xmin, ymax);
}

/** create geometry elements shared by all VSGVruiTexturedBackgrounds
 */
void VSGVruiTexturedBackground::createSharedLists()
{
    if (normal.get() == nullptr)
    {
        //normal = vec3Array::create();
        //(*normal)[0].set(0.0f, 0.0f, 1.0f);
    }
}

/** create the geometry
 */
void VSGVruiTexturedBackground::createGeometry()
{
    if (myDCS)
        return;
    auto trans = vsg::MatrixTransform::create();
    myDCS = new VSGVruiTransformNode(trans);

    tex = background->getCurrentTextures();

    if (tex)
    {
        createTexturesFromArrays(tex->normalTextureImage,
                                 tex->highlightedTextureImage,
                                 tex->disabledTextureImage,
                                 tex->comp, tex->s, tex->t, tex->r);
    }
    else
    {
        createTexturesFromFiles();
    }
    createGeode();
    if (tex->comp == 4) // texture with alpha
    { //FIXME
        //     state->setMode(PFSTATE_TRANSPARENCY, PFTR_BLEND_ALPHA);
        //     highlightState->setMode(PFSTATE_TRANSPARENCY, PFTR_BLEND_ALPHA);
        //     disabledState->setMode(PFSTATE_TRANSPARENCY, PFTR_BLEND_ALPHA);
    }

    ((vsg::MatrixTransform *)(myDCS->getNodePtr()))->addChild(geometryNode);
}

/** create texture from files
 */
void VSGVruiTexturedBackground::createTexturesFromFiles()
{
   /* VSGVruiTexture* oTex = dynamic_cast<VSGVruiTexture*>(vruiRendererInterface::the()->createTexture(background->getNormalTexName()));
    texNormal = oTex->getTexture();
    vruiRendererInterface::the()->deleteTexture(oTex);

    if (texNormal.valid())
    {
        texNormal->setNumMipmapLevels(MipMapLevels);
        texNormal->setFilter(Texture::MIN_FILTER, vsg::Texture::LINEAR_MIPMAP_NEAREST);
        texNormal->setFilter(Texture::MAG_FILTER, vsg::Texture::NEAREST);
        texNormal->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
        texNormal->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
    }
    else
    {
        VRUILOG("VSGVruiTransformNode::createTexturesFromFiles err: normal texture image '"
                << background->getNormalTexName() << "' not loaded")
    }

    oTex = dynamic_cast<VSGVruiTexture *>(vruiRendererInterface::the()->createTexture(background->getHighlightTexName()));
    texHighlighted = oTex->getTexture();
    vruiRendererInterface::the()->deleteTexture(oTex);

    if (texHighlighted.valid())
    {
        texHighlighted->setNumMipmapLevels(MipMapLevels);
        texHighlighted->setFilter(Texture::MIN_FILTER, vsg::Texture::LINEAR_MIPMAP_NEAREST);
        texHighlighted->setFilter(Texture::MAG_FILTER, vsg::Texture::NEAREST);
        texHighlighted->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
        texHighlighted->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
    }
    else
    {
        VRUILOG("VSGVruiTransformNode::createTexturesFromFiles err: highlight texture image '"
                << background->getHighlightTexName() << "' not loaded")
        texHighlighted = texNormal;
    }

    oTex = dynamic_cast<VSGVruiTexture *>(vruiRendererInterface::the()->createTexture(background->getDisabledTexName()));
    texDisabled = oTex->getTexture();
    vruiRendererInterface::the()->deleteTexture(oTex);

    if (texDisabled.valid())
    {
        texDisabled->setNumMipmapLevels(MipMapLevels);
        texDisabled->setFilter(Texture::MIN_FILTER, vsg::Texture::LINEAR_MIPMAP_NEAREST);
        texDisabled->setFilter(Texture::MAG_FILTER, vsg::Texture::NEAREST);
        texDisabled->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
        texDisabled->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
    }
    else
    {
        VRUILOG("VSGVruiTransformNode::createTexturesFromFiles err: disabled texture image '"
                << background->getDisabledTexName() << "' not loaded")
        texDisabled = texNormal;
    }*/
}

/** create texture from arrays
 */
void VSGVruiTexturedBackground::createTexturesFromArrays(const uint *normalImage,
                                                         const uint *highlightImage,
                                                         const uint *disabledImage,
                                                         int comp, int ns, int nt, int nr)
{
   /* texNormal = new Texture2D();

    GLenum pixel_format = comp == 3 ? GL_RGB : GL_RGBA;

    if (texNormal.valid())
    {
        ref_ptr<Image> image = new Image();

        image->setImage(ns, nt, nr, pixel_format, pixel_format, GL_UNSIGNED_BYTE,
                        (unsigned char *)normalImage, Image::NO_DELETE, comp);
        texNormal->setImage(image.get());
        texNormal->setNumMipmapLevels(MipMapLevels);
        texNormal->setFilter(Texture::MIN_FILTER, vsg::Texture::LINEAR_MIPMAP_NEAREST);
        texNormal->setFilter(Texture::MAG_FILTER, vsg::Texture::NEAREST);
        texNormal->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
        texNormal->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
    }
    else
    {
        VRUILOG("VSGVruiTransformNode::createTexturesFromArrays err: normal texture image creation error")
    }

    if (highlightImage != 0)
    {
        texHighlighted = new Texture2D();
        if (texHighlighted.valid())
        {
            ref_ptr<Image> image = new Image();
            image->setImage(ns, nt, nr, pixel_format, pixel_format, GL_UNSIGNED_BYTE,
                            (unsigned char *)highlightImage, Image::NO_DELETE, comp);
            texHighlighted->setImage(image.get());
            texHighlighted->setNumMipmapLevels(MipMapLevels);
            texHighlighted->setFilter(Texture::MIN_FILTER, vsg::Texture::LINEAR_MIPMAP_NEAREST);
            texHighlighted->setFilter(Texture::MAG_FILTER, vsg::Texture::NEAREST);
            texHighlighted->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
            texHighlighted->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
        }
    }
    else
    {
        texHighlighted = texNormal;
    }

    if (disabledImage != 0)
    {
        texDisabled = new Texture2D();
        if (texDisabled.valid())
        {
            ref_ptr<Image> image = new Image();
            image->setImage(ns, nt, nr, pixel_format, pixel_format, GL_UNSIGNED_BYTE,
                            (unsigned char *)disabledImage, Image::NO_DELETE, comp);
            texDisabled->setImage(image.get());
            texDisabled->setNumMipmapLevels(MipMapLevels);
            texDisabled->setFilter(Texture::MIN_FILTER, vsg::Texture::LINEAR_MIPMAP_NEAREST);
            texDisabled->setFilter(Texture::MAG_FILTER, vsg::Texture::NEAREST);
            texDisabled->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
            texDisabled->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
        }
    }
    else
    {
        texDisabled = texNormal;
    }*/
}

/** create geometry and texture objects
 */
void VSGVruiTexturedBackground::createGeode()
{
    createSharedLists();

   /* coord = new vec3Array(4);

    (*coord)[0].set(0, 0, 0);
    (*coord)[1].set(60, 0, 0);
    (*coord)[2].set(60, 60, 0);
    (*coord)[3].set(0, 60, 0);

    texcoord = new vec2Array(4);

    (*texcoord)[0].set(0.0, 0.0);
    (*texcoord)[1].set(1.0, 0.0);
    (*texcoord)[2].set(1.0, 1.0);
    (*texcoord)[3].set(0.0, 1.0);

    state = VSGVruiPresets::makeStateSet(coUIElement::WHITE);
    highlightState = VSGVruiPresets::makeStateSet(coUIElement::WHITE);
    disabledState = VSGVruiPresets::makeStateSet(coUIElement::WHITE);

    state->setTextureAttributeAndModes(0, texNormal.get(), StateAttribute::ON | StateAttribute::PROTECTED);
    state->setTextureAttributeAndModes(0, VSGVruiPresets::getTexEnvModulate(), StateAttribute::ON | StateAttribute::PROTECTED);

    highlightState->setTextureAttributeAndModes(0, texHighlighted.get(), StateAttribute::ON | StateAttribute::PROTECTED);
    highlightState->setTextureAttributeAndModes(0, VSGVruiPresets::getTexEnvModulate(), StateAttribute::ON | StateAttribute::PROTECTED);

    disabledState->setTextureAttributeAndModes(0, texDisabled.get(), StateAttribute::ON | StateAttribute::PROTECTED);
    disabledState->setTextureAttributeAndModes(0, VSGVruiPresets::getTexEnvModulate(), StateAttribute::ON | StateAttribute::PROTECTED);

    state->setMode(GL_BLEND, StateAttribute::ON | StateAttribute::PROTECTED);
    highlightState->setMode(GL_BLEND, StateAttribute::ON | StateAttribute::PROTECTED);
    disabledState->setMode(GL_BLEND, StateAttribute::ON | StateAttribute::PROTECTED);
    geometry = new Geometry();
    geometry->setVertexArray(coord.get());
    geometry->addPrimitiveSet(new DrawArrays(PrimitiveSet::QUADS, 0, 4));
    geometry->setNormalArray(normal.get());
    geometry->setNormalBinding(Geometry::BIND_OVERALL);
    geometry->setTexCoordArray(0, texcoord.get());
    geometryNode = new Geode();
    geometryNode->setStateSet(state.get());
    geometryNode->addDrawable(geometry.get());*/
    resizeGeometry();
}

/** Set activation state of this background and all its children.
  if this background is disabled, the color is always the
  disabled color, regardless of the highlighted state
  @param en true = elements enabled
*/
void VSGVruiTexturedBackground::setEnabled(bool en)
{
   /* if (en)
    {
        if (background->isHighlighted())
        {
            geometryNode->setStateSet(highlightState.get());
        }
        else
        {
            geometryNode->setStateSet(state.get());
        }
    }
    else
    {
        geometryNode->setStateSet(disabledState.get());
    }*/
}

void VSGVruiTexturedBackground::setHighlighted(bool hl)
{
  /*  if (background->isEnabled())
    {
        if (hl)
        {
            geometryNode->setStateSet(highlightState.get());
        }
        else
        {
            geometryNode->setStateSet(state.get());
        }
    }
    else
    {
        geometryNode->setStateSet(disabledState.get());
    }*/
}
}
