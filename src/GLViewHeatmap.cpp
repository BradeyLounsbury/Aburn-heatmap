#include "GLViewHeatmap.h"

#include "WorldList.h" //This is where we place all of our WOs
#include "ManagerOpenGLState.h" //We can change OpenGL State attributes with this
#include "Axes.h" //We can set Axes to on/off with this
#include "PhysicsEngineODE.h"

//Different WO used by this module
#include "WO.h"
#include "WOGrid.h"
#include "WOStatic.h"
#include "WOStaticPlane.h"
#include "WOStaticTrimesh.h"
#include "WOTrimesh.h"
#include "WOHumanCyborg.h"
#include "WOHumanCal3DPaladin.h"
#include "WOWayPointSpherical.h"
#include "WOLight.h"
#include "WOSkyBox.h"
#include "WOCar1970sBeater.h"
#include "Camera.h"
#include "CameraStandard.h"
#include "CameraChaseActorSmooth.h"
#include "CameraChaseActorAbsNormal.h"
#include "CameraChaseActorRelNormal.h"
#include "Model.h"
#include "ModelDataShared.h"
#include "ModelMesh.h"
#include "ModelMeshDataShared.h"
#include "ModelMeshSkin.h"
#include "WONVStaticPlane.h"
#include "WONVPhysX.h"
#include "WONVDynSphere.h"
#include "WOImGui.h" //GUI Demos also need to #include "AftrImGuiIncludes.h"
#include "AftrImGuiIncludes.h"
#include "AftrGLRendererBase.h"
#include "OpenSimplexNoise.h"
#include "ModelMeshRenderDataGenerator.h"
#include "GLSLShaderDefaultGL32.h"
#include "GLSLUniform.h"

using namespace Aftr;

static int seed = 0;
static float scale = 0.0;
static bool load_heatmap, animate, pulsate, auto_load = false;
static int grid_id = -1;

GLViewHeatmap* GLViewHeatmap::New( const std::vector< std::string >& args )
{
   GLViewHeatmap* glv = new GLViewHeatmap( args );
   glv->init( Aftr::GRAVITY, Vector( 0, 0, -1.0f ), "aftr.conf", PHYSICS_ENGINE_TYPE::petODE );
   glv->onCreate();
   return glv;
}


GLViewHeatmap::GLViewHeatmap( const std::vector< std::string >& args ) : GLView( args )
{
   //Initialize any member variables that need to be used inside of LoadMap() here.
   //Note: At this point, the Managers are not yet initialized. The Engine initialization
   //occurs immediately after this method returns (see GLViewHeatmap::New() for
   //reference). Then the engine invoke's GLView::loadMap() for this module.
   //After loadMap() returns, GLView::onCreate is finally invoked.

   //The order of execution of a module startup:
   //GLView::New() is invoked:
   //    calls GLView::init()
   //       calls GLView::loadMap() (as well as initializing the engine's Managers)
   //    calls GLView::onCreate()

   //GLViewHeatmap::onCreate() is invoked after this module's LoadMap() is completed.
}


void GLViewHeatmap::onCreate()
{
   //GLViewHeatmap::onCreate() is invoked after this module's LoadMap() is completed.
   //At this point, all the managers are initialized. That is, the engine is fully initialized.


   if( this->pe != NULL )
   {
      //optionally, change gravity direction and magnitude here
      //The user could load these values from the module's aftr.conf
      this->pe->setGravityNormalizedVector( Vector( 0,0,-1.0f ) );
      this->pe->setGravityScalar( Aftr::GRAVITY );
   }
   this->setActorChaseType( STANDARDEZNAV ); //Default is STANDARDEZNAV mode
   //this->setNumPhysicsStepsPerRender( 0 ); //pause physics engine on start up; will remain paused till set to 1
}

void set_grid_id(unsigned int id) {
    grid_id = id;
}

void pulsate_heatmap(WO* wo, int section, int& max, int modifier) {
    OpenSimplexNoise noise(seed);

    std::vector<float> verts;
    auto og_verts = wo->getModel()->getCompositeVertexList();
    auto v_offset = (og_verts.size() / 10) * section;
    for (size_t i = v_offset; i < (og_verts.size() / 10) * (section + 1); ++i) {
        verts.push_back(float(og_verts[i][0]));
        verts.push_back(float(og_verts[i][1]));
        verts.push_back(float(og_verts[i][2]));
    }

    for (int i = 0; i < verts.size(); i += 3) {
        float z = 0;
        auto n = noise.Evaluate(verts[i] * 256, verts[i + 1] * 256) * (scale + modifier);
        if (n > 0) {
            if (n > max) max = n;
            z = n;
        }
        //z = 0;
        verts[i + 2] = z;
    }
    float* va = verts.data();

    auto loc = glGetUniformLocation(34, "max_height");
    glProgramUniform1f(34, loc, max);

    auto stride = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVtxStride();
    auto size = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVtxSize();
    auto offset = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVertsOffset();

    auto buffer = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVBOVtx();

    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    float sub_va[3]{ va[0], va[1], va[2] };
    int pos = 0;
    for (auto i = offset * stride * section; i < offset * stride * (section + 1); i += stride) {
        sub_va[pos % 3] = va[pos];
        sub_va[pos % 3 + 1] = va[pos + 1];
        sub_va[pos % 3 + 2] = va[pos + 2];
        glBufferSubData(GL_ARRAY_BUFFER, i, sizeof(float) * 3, sub_va);
        pos += 3;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void generate_heatmap_step(WO* wo, int step, int& max) {
    OpenSimplexNoise noise(seed);

    float verts[3];
    auto og_verts = wo->getModel()->getCompositeVertexList();
    verts[0] = float(og_verts[step][0]);
    verts[1] = float(og_verts[step][1]);
    verts[2] = float(og_verts[step][2]);

    float z = 0;
    auto n = noise.Evaluate(verts[0] * 256, verts[1] * 256) * scale;
    if (n > 0) {
        if (n > max) max = n;
        z = n;
    }
    verts[2] = z;

    auto loc = glGetUniformLocation(34, "max_height");
    glProgramUniform1f(34, loc, max);

    auto stride = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVtxStride();
    auto size = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVtxSize();
    auto offset = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVertsOffset();

    auto buffer = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVBOVtx();

    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    glBufferSubData(GL_ARRAY_BUFFER, (step * stride) + offset, sizeof(float) * 3, verts);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void generate_heatmap(WO* wo) {
    OpenSimplexNoise noise(seed);

    std::vector<float> verts;
    auto og_verts = wo->getModel()->getCompositeVertexList();
    for (size_t i = 0; i < og_verts.size(); ++i) {
        verts.push_back(float(og_verts[i][0]));
        verts.push_back(float(og_verts[i][1]));
        verts.push_back(float(og_verts[i][2]));
    }

    float max = 0;
    for (int i = 0; i < verts.size(); i += 3) {
        float z = 0;
        auto n = noise.Evaluate(verts[i] * 256, verts[i + 1] * 256) * scale;
        if (n > 0) {
            if (n > max) max = n;
            z = n;
        }
        //z = 0;
        verts[i + 2] = z;
    }
    float* va = verts.data();

    auto loc = glGetUniformLocation(34, "max_height");
    glProgramUniform1f(34, loc, max);

    auto stride = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVtxStride();
    auto size = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVtxSize();
    auto offset = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVertsOffset();

    auto buffer = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getMeshDataShared()->getModelMeshRenderData(MESH_SHADING_TYPE::mstSMOOTH, 4)->getVBOVtx();

    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    float sub_va[3]{ va[0], va[1], va[2] };
    int pos = 0;
    for (auto i = offset; i < size * stride; i += stride) {
        sub_va[pos % 3] = va[pos];
        sub_va[pos % 3 + 1] = va[pos + 1];
        sub_va[pos % 3 + 2] = va[pos + 2];
        glBufferSubData(GL_ARRAY_BUFFER, i, sizeof(float) * 3, sub_va);
        pos += 3;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLViewHeatmap::~GLViewHeatmap()
{
   //Implicitly calls GLView::~GLView()
}

static int counter, step, max = 0;
void GLViewHeatmap::updateWorld()
{
   GLView::updateWorld(); //Just call the parent's update world first.
                          //If you want to add additional functionality, do it after
                          //this call.

   if (auto_load) {
       generate_heatmap(this->worldLst->getWOByID(grid_id));
       load_heatmap = false;
       animate = false;
       pulsate = false;
   }

   if (load_heatmap) {
       generate_heatmap(this->worldLst->getWOByID(grid_id));
       load_heatmap = false;
       animate = false;
       pulsate = false;
   }

   if (animate) {
       generate_heatmap_step(this->worldLst->getWOByID(grid_id), step, max);
       step++;
       pulsate = false;
       /*if (counter == 0) {
           max = generate_heatmap_step(this->worldLst->getWOByID(grid_id), step, max);
           step++;
           counter = 5;
       }
       else {
           counter--;
       }*/
   }

   /*if (pulsate) {
       animate = false;
       int section = rand() % 10;
       int modifier = rand() % 2;
       modifier < 0 ? modifier = -1 : modifier = 1;
       pulsate_heatmap(this->worldLst->getWOByID(grid_id), section, max, modifier);
   }*/
}


void GLViewHeatmap::onResizeWindow( GLsizei width, GLsizei height )
{
   GLView::onResizeWindow( width, height ); //call parent's resize method.
}


void GLViewHeatmap::onMouseDown( const SDL_MouseButtonEvent& e )
{
   GLView::onMouseDown( e );
}


void GLViewHeatmap::onMouseUp( const SDL_MouseButtonEvent& e )
{
   GLView::onMouseUp( e );
}


void GLViewHeatmap::onMouseMove( const SDL_MouseMotionEvent& e )
{
   GLView::onMouseMove( e );
}


void GLViewHeatmap::onKeyDown( const SDL_KeyboardEvent& key )
{
   GLView::onKeyDown( key );
   if( key.keysym.sym == SDLK_0 )
      this->setNumPhysicsStepsPerRender( 1 );
}


void GLViewHeatmap::onKeyUp( const SDL_KeyboardEvent& key )
{
   GLView::onKeyUp( key );
}


void Aftr::GLViewHeatmap::loadMap()
{
   this->worldLst = new WorldList(); //WorldList is a 'smart' vector that is used to store WO*'s
   this->actorLst = new WorldList();
   this->netLst = new WorldList();

   ManagerOpenGLState::GL_CLIPPING_PLANE = 1000.0;
   ManagerOpenGLState::GL_NEAR_PLANE = 0.1f;
   ManagerOpenGLState::enableFrustumCulling = false;
   Axes::isVisible = false;
   this->glRenderer->isUsingShadowMapping( false ); //set to TRUE to enable shadow mapping, must be using GL 3.2+

   this->cam->setPosition( 0,100,20 );

   std::string shinyRedPlasticCube( ManagerEnvironmentConfiguration::getSMM() + "/models/cube4x4x4redShinyPlastic_pp.wrl" );
   std::string wheeledCar( ManagerEnvironmentConfiguration::getSMM() + "/models/rcx_treads.wrl" );
   std::string grass( ManagerEnvironmentConfiguration::getSMM() + "/models/grassFloor400x400_pp.wrl" );
   std::string human( ManagerEnvironmentConfiguration::getSMM() + "/models/human_chest.wrl" );
   std::string grid(ManagerEnvironmentConfiguration::getLMM() + "/models/new_grid_small.obj");

   OpenSimplexNoise noise;

   /*for (int i = 0; i < 50; i++) {
       for (int j = 0; j < 50; j++) {
           auto z = (noise.Evaluate(i * 256, j * 256) * 5) + 5;

           auto cubeWO = WO::New(shinyRedPlasticCube, Vector(0.1, 0.1, 0.1), MESH_SHADING_TYPE::mstFLAT);
           cubeWO->setPosition(i, j, z);
           this->worldLst->push_back(cubeWO);
       }
   }*/

   //SkyBox Textures readily available
   std::vector< std::string > skyBoxImageNames; //vector to store texture paths
   skyBoxImageNames.push_back(ManagerEnvironmentConfiguration::getLMM() + "/models/Solid_black.png");
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_water+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_dust+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_mountains+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_winter+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/early_morning+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_afternoon+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_cloudy+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_cloudy3+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_day+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_day2+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_deepsun+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_evening+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_morning+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_morning2+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_noon+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_warp+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_Hubble_Nebula+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_gray_matter+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_easter+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_hot_nebula+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_ice_field+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_lemon_lime+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_milk_chocolate+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_solar_bloom+6.jpg" );
   //skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_thick_rb+6.jpg" );

   {
      //Create a light
      float ga = 0.1f; //Global Ambient Light level for this module
      ManagerLight::setGlobalAmbientLight( aftrColor4f( ga, ga, ga, 1.0f ) );
      WOLight* light = WOLight::New();
      light->isDirectionalLight( true );
      light->setPosition( Vector( 0, 0, 100 ) );
      //Set the light's display matrix such that it casts light in a direction parallel to the -z axis (ie, downwards as though it was "high noon")
      //for shadow mapping to work, this->glRenderer->isUsingShadowMapping( true ), must be invoked.
      light->getModel()->setDisplayMatrix( Mat4::rotateIdentityMat( { 0, 1, 0 }, 90.0f * Aftr::DEGtoRAD ) );
      light->setLabel( "Light" );
      worldLst->push_back( light );
   }

   {
      //Create the SkyBox
      WO* wo = WOSkyBox::New( skyBoxImageNames.at( 0 ), this->getCameraPtrPtr() );
      wo->setPosition( Vector( 0, 0, 0 ) );
      wo->setLabel( "Sky Box" );
      wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
      worldLst->push_back( wo );
   }

   {
       WO* wo = WO::New(grid, Vector(50,50,50), MESH_SHADING_TYPE::mstSMOOTH);
       

       wo->setPosition(Vector(50, 50, 0));
       wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
       wo->upon_async_model_loaded([wo]()
           {
               ModelMeshSkin& grassSkin = wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getSkins().at(0);

               grassSkin.getMultiTextureSet().at(0).setTexRepeats(5.0f);
               grassSkin.setAmbient(aftrColor4f(0.4f, 0.4f, 0.4f, 1.0f)); //Color of object when it is not in any light
               grassSkin.setDiffuse(aftrColor4f(1.0f, 1.0f, 1.0f, 1.0f)); //Diffuse color components (ie, matte shading color of this object)
               grassSkin.setSpecular(aftrColor4f(0.4f, 0.4f, 0.4f, 1.0f)); //Specular color component (ie, how "shiney" it is)
               grassSkin.setSpecularCoefficient(10); // How "sharp" are the specular highlights (bigger is sharper, 1000 is very sharp, 10 is very dull)

               std::cout << "\nSKIN: " << grassSkin.toString() << std::endl;

               class GLSLShaderGrid : public GLSLShaderDefaultGL32
               {
               protected:
                   GLSLShaderGrid(GLSLShaderDataShared* s) : GLSLShaderDefaultGL32(s)
                   {

                   }
               public:
                   static GLSLShaderGrid* New()
                   {
                       std::string vert = ManagerEnvironmentConfiguration::getLMM() + "/shaders/defaultGL32.vert";
                       std::string frag = ManagerEnvironmentConfiguration::getLMM() + "/shaders/defaultGL32.frag";

                       GLSLShaderDataShared* data = ManagerShader::loadShaderDataShared(vert, frag);
                       if (data == nullptr)
                           return nullptr;

                       GLSLShaderGrid* ptr = new GLSLShaderGrid(data);
                       return ptr;
                   }
               };

               ModelMeshSkin skin(ManagerTex::loadTexAsync(ManagerEnvironmentConfiguration::getLMM() + "/models/Solid_black.png").value(), GLSLShaderGrid::New());
               skin.setMeshShadingType(MESH_SHADING_TYPE::mstSMOOTH);
               wo->getModel()->getSkins().push_back(std::move(skin));

               wo->getModel()->useNextSkin();
           });

       wo->setLabel("Grid");
       wo->isVisible = true;
       set_grid_id(wo->getID());
       worldLst->push_back(wo);
   }

   //{ 
   //   ////Create the infinite grass plane (the floor)
   //   WO* wo = WO::New( grass, Vector( 1, 1, 1 ), MESH_SHADING_TYPE::mstFLAT );

   //   /*OpenSimplexNoise noise;
   //   auto& vertices = wo->getModel()->getCompositeVertexList();
   //   auto z_vec = vertices;
   //   for (int i = 0; i < vertices.size(); i++) {
   //       auto z = (noise.Evaluate(vertices[i][0] * 256, vertices[i][1] * 256) * 100) + 5;
   //       z_vec[i][2] = z;
   //   }*/
   //   wo->setPosition( Vector( 0, 0, 0 ) );
   //   wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
   //   wo->upon_async_model_loaded( [wo]()
   //      {
   //         ModelMeshSkin& grassSkin = wo->getModel()->getModelDataShared()->getModelMeshes().at( 0 )->getSkins().at( 0 );
   //         grassSkin.getMultiTextureSet().at( 0 ).setTexRepeats( 5.0f );
   //         grassSkin.setAmbient( aftrColor4f( 0.4f, 0.4f, 0.4f, 1.0f ) ); //Color of object when it is not in any light
   //         grassSkin.setDiffuse( aftrColor4f( 1.0f, 1.0f, 1.0f, 1.0f ) ); //Diffuse color components (ie, matte shading color of this object)
   //         grassSkin.setSpecular( aftrColor4f( 0.4f, 0.4f, 0.4f, 1.0f ) ); //Specular color component (ie, how "shiney" it is)
   //         grassSkin.setSpecularCoefficient( 10 ); // How "sharp" are the specular highlights (bigger is sharper, 1000 is very sharp, 10 is very dull)
   //      } );
   //   
   //   wo->setLabel( "Grass" );
   //   worldLst->push_back( wo );
   //}

   //{
   //   //Create the infinite grass plane that uses the Open Dynamics Engine (ODE)
   //   WO* wo = WOStatic::New( grass, Vector(1,1,1), MESH_SHADING_TYPE::mstFLAT );
   //   ((WOStatic*)wo)->setODEPrimType( ODE_PRIM_TYPE::PLANE );
   //   wo->setPosition( Vector(0,0,0) );
   //   wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
   //   wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getSkins().at(0).getMultiTextureSet().at(0)->setTextureRepeats( 5.0f );
   //   wo->setLabel( "Grass" );
   //   worldLst->push_back( wo );
   //}

   //{
   //   //Create the infinite grass plane that uses NVIDIAPhysX(the floor)
   //   WO* wo = WONVStaticPlane::New( grass, Vector( 1, 1, 1 ), MESH_SHADING_TYPE::mstFLAT );
   //   wo->setPosition( Vector( 0, 0, 0 ) );
   //   wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
   //   wo->getModel()->getModelDataShared()->getModelMeshes().at( 0 )->getSkins().at( 0 ).getMultiTextureSet().at( 0 )->setTextureRepeats( 5.0f );
   //   wo->setLabel( "Grass" );
   //   worldLst->push_back( wo );
   //}

   //{
   //   //Create the infinite grass plane (the floor)
   //   WO* wo = WONVPhysX::New( shinyRedPlasticCube, Vector( 1, 1, 1 ), MESH_SHADING_TYPE::mstFLAT );
   //   wo->setPosition( Vector( 0, 0, 50.0f ) );
   //   wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
   //   wo->setLabel( "Grass" );
   //   worldLst->push_back( wo );
   //}

   //{
   //   WO* wo = WONVPhysX::New( shinyRedPlasticCube, Vector( 1, 1, 1 ), MESH_SHADING_TYPE::mstFLAT );
   //   wo->setPosition( Vector( 0, 0.5f, 75.0f ) );
   //   wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
   //   wo->setLabel( "Grass" );
   //   worldLst->push_back( wo );
   //}

   //{
   //   WO* wo = WONVDynSphere::New( ManagerEnvironmentConfiguration::getVariableValue( "sharedmultimediapath" ) + "/models/sphereRp5.wrl", Vector( 1.0f, 1.0f, 1.0f ), mstSMOOTH );
   //   wo->setPosition( 0, 0, 100.0f );
   //   wo->setLabel( "Sphere" );
   //   this->worldLst->push_back( wo );
   //}

   //{
   //   WO* wo = WOHumanCal3DPaladin::New( Vector( .5, 1, 1 ), 100 );
   //   ((WOHumanCal3DPaladin*)wo)->rayIsDrawn = false; //hide the "leg ray"
   //   ((WOHumanCal3DPaladin*)wo)->isVisible = false; //hide the Bounding Shell
   //   wo->setPosition( Vector( 20, 20, 20 ) );
   //   wo->setLabel( "Paladin" );
   //   worldLst->push_back( wo );
   //   actorLst->push_back( wo );
   //   netLst->push_back( wo );
   //   this->setActor( wo );
   //}
   //
   //{
   //   WO* wo = WOHumanCyborg::New( Vector( .5, 1.25, 1 ), 100 );
   //   wo->setPosition( Vector( 20, 10, 20 ) );
   //   wo->isVisible = false; //hide the WOHuman's bounding box
   //   ((WOHuman*)wo)->rayIsDrawn = false; //show the 'leg' ray
   //   wo->setLabel( "Human Cyborg" );
   //   worldLst->push_back( wo );
   //   actorLst->push_back( wo ); //Push the WOHuman as an actor
   //   netLst->push_back( wo );
   //   this->setActor( wo ); //Start module where human is the actor
   //}

   //{
   //   //Create and insert the WOWheeledVehicle
   //   std::vector< std::string > wheels;
   //   std::string wheelStr( "../../../shared/mm/models/WOCar1970sBeaterTire.wrl" );
   //   wheels.push_back( wheelStr );
   //   wheels.push_back( wheelStr );
   //   wheels.push_back( wheelStr );
   //   wheels.push_back( wheelStr );
   //   WO* wo = WOCar1970sBeater::New( "../../../shared/mm/models/WOCar1970sBeater.wrl", wheels );
   //   wo->setPosition( Vector( 5, -15, 20 ) );
   //   wo->setLabel( "Car 1970s Beater" );
   //   ((WOODE*)wo)->mass = 200;
   //   worldLst->push_back( wo );
   //   actorLst->push_back( wo );
   //   this->setActor( wo );
   //   netLst->push_back( wo );
   //}
   
   //Make a Dear Im Gui instance via the WOImGui in the engine... This calls
   //the default Dear ImGui demo that shows all the features... To create your own,
   //inherit from WOImGui and override WOImGui::drawImGui_for_this_frame(...) (among any others you need).
   WOImGui* gui = WOImGui::New( nullptr );
   gui->setLabel( "My Gui" );
   gui->subscribe_drawImGuiWidget(
      [this, gui]() //this is a lambda, the capture clause is in [], the input argument list is in (), and the body is in {}
      {
           ImGui::Begin("Heatmap Gui");

           ImGui::SliderInt("Seed", &seed, 0, 100);
           ImGui::SliderFloat("Scale", &scale, 0, 100);

           ImGui::Separator();

           if (ImGui::Button("Load Heatmap")) {
               load_heatmap = true;
           }
           ImGui::SameLine();
           if (ImGui::Button("Animate")) {
               animate = true;
           }
           ImGui::SameLine();
           ImGui::Checkbox("Auto Load Heatmap", &auto_load);
           /*ImGui::SameLine();
           if (ImGui::Button("Pulsate")) {
               pulsate = true;
           }*/

           ImGui::End();
      } );
   this->worldLst->push_back( gui );

   //createHeatmapWayPoints();
}


void GLViewHeatmap::createHeatmapWayPoints()
{
   // Create a waypoint with a radius of 3, a frequency of 5 seconds, activated by GLView's camera, and is visible.
   WayPointParametersBase params(this);
   params.frequency = 5000;
   params.useCamera = true;
   params.visible = true;
   WOWayPointSpherical* wayPt = WOWayPointSpherical::New( params, 3 );
   wayPt->setPosition( Vector( 50, 0, 3 ) );
   worldLst->push_back( wayPt );
}
