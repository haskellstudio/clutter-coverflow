#include <math.h>
#include "coverflow.h"
#include "cover.h"

#define FAILURE        1
#define SUCCESS        0

/*
#define PLACES         9
#define CURRENT        4
#define COUNT         10

#define WIDTH        900.0
#define HEIGHT       500.0
#define LEFT_MARGIN    0.0
#define SEPARATION    30.0
#define ROTATION      65.0
#define COVER_WIDTH  300.0
#define COVER_HEIGHT 300.0
#define MIDDLE_ZOOM   40.0
#define MIDDLE_DEPTH 100.0
#define CAMERA_ANGLE  20.0
#define DURATION     200.0
#define OVER_OFFSET   10.0
#define FLOOR         40.0
#define TRANSPARENCY   8.0
*/

#define PLACES        35
#define CURRENT       17
#define COUNT         40

#define WIDTH        800.0
#define HEIGHT       500.0
#define LEFT_MARGIN    0.0
#define SEPARATION    15.0
#define ROTATION      65.0
#define COVER_WIDTH  150.0
#define COVER_HEIGHT 150.0
#define MIDDLE_ZOOM   70.0
#define MIDDLE_DEPTH 200.0
#define CAMERA_ANGLE  40.0
#define DURATION     200.0
#define OVER_OFFSET   10.0
#define FLOOR         40.0
#define TRANSPARENCY  32.0

static gint current = CURRENT;

/* initialize cover values */
static void
initialize (Cover *cover, gint position)
{
  ClutterActor *actor = cover->cover;
  ClutterActor *clone = cover->reflection;

  gfloat width = COVER_WIDTH;
  gfloat height = COVER_HEIGHT;

  gfloat depth, offset, factor, middle;

  clutter_actor_set_size (actor, width, height);
  clutter_actor_set_size (clone, width, height);

	middle = ceil (PLACES / 2.0);

  if (position <= middle)
    depth = position;
  else
    depth = middle - (position - middle);

  if (position < middle)
  {
    factor = 1;
    offset = LEFT_MARGIN;
  }
  else if (position > middle)
  {
    factor = -1;
    offset = width + LEFT_MARGIN;
  }
  else /* position == middle */
  {
    factor = 0;
    offset = (width + LEFT_MARGIN) * 0.5;
  }

  clutter_actor_set_position (actor, SEPARATION * position + offset, FLOOR);
  clutter_actor_set_rotation (actor, CLUTTER_Y_AXIS, ROTATION * factor, width * 0.5, 0.0, 0.0);
  clutter_actor_set_depth (actor, depth);

  clutter_actor_add_constraint (clone, clutter_bind_constraint_new (actor, CLUTTER_BIND_X, 0.0));
  clutter_actor_set_position (clone, 0.0, height + FLOOR);
  clutter_actor_set_rotation (clone, CLUTTER_Y_AXIS, ROTATION * factor, width * 0.5, 0.0, 0.0);
  clutter_actor_set_depth (clone, depth);

  if (position == middle)
  {
    clutter_actor_set_size (actor, width + MIDDLE_ZOOM, height + MIDDLE_ZOOM);
    clutter_actor_move_by (actor, MIDDLE_ZOOM / -2.0, MIDDLE_ZOOM / -2.0);
  	//clutter_actor_set_rotation (actor, CLUTTER_X_AXIS, 20, 0.0, 0.0, 0.0);
    clutter_actor_set_depth (actor, MIDDLE_DEPTH);

    clutter_actor_set_size (clone, width + MIDDLE_ZOOM, height + MIDDLE_ZOOM);
    clutter_actor_move_by (clone, MIDDLE_ZOOM / -2.0, MIDDLE_ZOOM / 2.0);
    clutter_actor_set_depth (clone, MIDDLE_DEPTH);
  }
}

/* computes the position of current cover to be placed on next place */
static int
get_position (int position, int next)
{
  int value = next - (ceil (PLACES / 2.0) - position);

  if (value < 0 || value > COUNT)
    value = -1;

  return value;
}

/* animation bind helper */
static void
animation_bind (ClutterAnimation *animation, const gchar *property, gfloat value)
{
  GValue g_value = {0};
  g_value_init (&g_value, G_TYPE_FLOAT);
  g_value_set_float (&g_value, value);
  clutter_animation_bind (animation, property, &g_value);
}

/* movement animation */
static void
move (ClutterActor *source, ClutterActor *target, ClutterTimeline *timeline)
{
  gfloat x, y, z, angle_x, angle_y, width, height, depth;
  ClutterAnimation *animation;

  clutter_actor_get_size (target, &width, &height);

  angle_x = clutter_actor_get_rotation (target, CLUTTER_X_AXIS, &x, &y, &z);
  angle_y = clutter_actor_get_rotation (target, CLUTTER_Y_AXIS, &x, &y, &z);
  x = clutter_actor_get_x (target);
  y = clutter_actor_get_y (target);
  depth = clutter_actor_get_depth (target);

  animation = clutter_animation_new ();
  clutter_animation_set_object (animation, G_OBJECT (source));
  clutter_animation_set_mode (animation, CLUTTER_EASE_IN_OUT_SINE);
  clutter_animation_set_timeline (animation, timeline);

  animation_bind (animation, "x", x);
  animation_bind (animation, "y", y);
  animation_bind (animation, "width", width);
  animation_bind (animation, "height", height);
  animation_bind (animation, "rotation-angle-x", angle_x);
  animation_bind (animation, "rotation-angle-y", angle_y);
  animation_bind (animation, "depth", depth);
}

/* move a cover */
static void
move_cover (Coverflow *coverflow, gint from, gint to, ClutterTimeline *timeline)
{
  if (to < 0 || to >= PLACES)
    return;

  Cover *source = g_slist_nth_data (coverflow->covers, from);
  Cover *target = g_slist_nth_data (coverflow->places, to);

  move (CLUTTER_ACTOR (source->cover), CLUTTER_ACTOR (target->cover), timeline);
  move (CLUTTER_ACTOR (source->reflection), CLUTTER_ACTOR (target->reflection), timeline);
}

/* move all covers in the coverflow */
static void
move_all (Coverflow *coverflow, gint to)
{
  ClutterTimeline *timeline = clutter_timeline_new (DURATION);
  int i;

  /* push all movements in a timeline */
  for (i = 0; i < PLACES; i++)
    move_cover (coverflow, i, get_position (i + 1, to), timeline);

  clutter_timeline_start (timeline);
}

/* keyboard event */
static gboolean
keyboard (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  g_signal_stop_emission_by_name (data, "key-press-event");

  ClutterBindingPool *pool;
  gboolean return_value;

  pool = clutter_binding_pool_find (G_OBJECT_TYPE_NAME (actor));

  return_value = clutter_binding_pool_activate (pool, clutter_event_get_key_symbol (event),
                                                clutter_event_get_state (event), G_OBJECT (actor));

  return return_value;
}

/* arrow right key handler */
static void
right (GObject *instance, const gchar *action, guint key, ClutterModifierType modifiers, gpointer data)
{
  move_all (data, ++current);
}

/* arrow left key handler */
static void
left (GObject *instance, const gchar *action, guint key, ClutterModifierType modifiers, gpointer data)
{
  move_all (data, --current);
}

/* quit (q/Q) key handler */
static void
quit (GObject *instance, const gchar *action, guint key, ClutterModifierType modifiers, gpointer data)
{
  clutter_main_quit ();
}

/* mouse events */
static gboolean
mouse_enter (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  clutter_actor_move_by (actor, 0.0, -OVER_OFFSET);
  clutter_actor_move_by (CLUTTER_ACTOR (data), 0.0, OVER_OFFSET);

  return TRUE;
}

static gboolean
mouse_leave (ClutterActor *actor, ClutterEvent *event, gpointer data)
{
  clutter_actor_move_by (actor, 0.0, OVER_OFFSET);
  clutter_actor_move_by (CLUTTER_ACTOR (data), 0.0, -OVER_OFFSET);

  return TRUE;
}

static void
paint_reflection (ClutterActor *actor, gpointer data)
{
  ClutterActor *source;
  CoglHandle material;
  ClutterActorBox box;
  gfloat width, height;
  CoglColor color_1, color_2;
  CoglTextureVertex vertices[4];
  guint8 opacity;

  source = clutter_clone_get_source (CLUTTER_CLONE (actor));

  if (source == NULL)
    goto out;

  material = clutter_texture_get_cogl_material (CLUTTER_TEXTURE (source));

  if (material == NULL)
    goto out;

  opacity = clutter_actor_get_paint_opacity (actor);

  clutter_actor_get_allocation_box (actor, &box);
  clutter_actor_box_get_size (&box, &width, &height);

  cogl_color_init_from_4f (&color_1, 1.0, 1.0, 1.0, opacity / TRANSPARENCY);
  cogl_color_premultiply (&color_1);

  cogl_color_init_from_4f (&color_2, 1.0, 1.0, 1.0, 0.0);
  cogl_color_premultiply (&color_2);

  vertices[0].x = 0;
  vertices[0].y = 0;
  vertices[0].z = 0;
  vertices[0].tx = 0.0;
  vertices[0].ty = 1.0;
  vertices[0].color = color_1;

  vertices[1].x = width;
  vertices[1].y = 0;
  vertices[1].z = 0;
  vertices[1].tx = 1.0;
  vertices[1].ty = 1.0;
  vertices[1].color = color_1;

  vertices[2].x = width;
  vertices[2].y = height;
  vertices[2].z = 0;
  vertices[2].tx = 1.0;
  vertices[2].ty = 0.0;
  vertices[2].color = color_2;

  vertices[3].x = 0;
  vertices[3].y = height;
  vertices[3].z = 0;
  vertices[3].tx = 0.0;
  vertices[3].ty = 0.0;
  vertices[3].color = color_2;

  cogl_set_source (material);
  cogl_polygon (vertices, 4, TRUE);

out:
  g_signal_stop_emission_by_name (actor, "paint");
}

int
main (int argc, char *argv[])
{
  Coverflow *coverflow;
  ClutterActor *stage;
  ClutterActor *group;
  GObjectClass *class;
  ClutterBindingPool *pool;
  gint i;

  ClutterColor background = { 0x0, 0x0, 0x0, 0xff };

  coverflow = g_new0 (Coverflow, 1);
  coverflow->covers = NULL;
  coverflow->places = NULL;

  if (clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return FAILURE;

  stage = clutter_stage_get_default ();

  clutter_stage_set_title (CLUTTER_STAGE (stage), "clutter-very-simple-coverflow");
  clutter_actor_set_size (stage, WIDTH, HEIGHT);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &background);

  group = clutter_group_new ();

  /* create the places for each cover and shadow visible in the coverflow */
  for (i = 1; i <= PLACES; i++)
  {
    Cover *place;

    place = g_new0 (Cover, 1);
    place->cover = clutter_rectangle_new ();
    place->reflection = clutter_clone_new (place->cover);

    coverflow->places = g_slist_append (coverflow->places, place);

    clutter_container_add (CLUTTER_CONTAINER (group), place->cover, place->reflection, NULL);

    initialize (place, i);

    clutter_actor_hide (place->cover);
    clutter_actor_hide (place->reflection);
  }

  /* create the covers */
  for (i = 0; i < COUNT; i++)
  {
    Cover *cover;
    gchar *filename;
    GError *error;

    cover = g_new0 (Cover, 1);
    cover->cover = clutter_texture_new ();
    cover->reflection = clutter_clone_new (cover->cover);

    coverflow->covers = g_slist_append (coverflow->covers, cover);

    filename = g_strdup_printf ("covers/%04d.png", i);
    clutter_actor_set_name (cover->cover, filename);

    error = NULL;
    clutter_texture_set_from_file (CLUTTER_TEXTURE (cover->cover), filename, &error);
    g_free (filename);

    if (error)
    {
      g_error ("Error al asignar la imágen a la textura: %s", error->message);
      g_error_free (error);
      return FAILURE;
    }

    clutter_actor_set_reactive (cover->cover, TRUE);
    clutter_texture_set_filter_quality (CLUTTER_TEXTURE (cover->cover), CLUTTER_TEXTURE_QUALITY_HIGH);
    clutter_texture_set_sync_size (CLUTTER_TEXTURE (cover->cover), TRUE);

    if (i < PLACES)
      initialize (cover, i + 1);
    else
    {
      clutter_actor_hide (CLUTTER_ACTOR (cover->reflection));
      clutter_actor_hide (CLUTTER_ACTOR (cover->cover));
    }

    clutter_container_add (CLUTTER_CONTAINER (group), cover->cover, cover->reflection, NULL);

    g_signal_connect (cover->reflection, "paint", G_CALLBACK (paint_reflection), NULL);
    g_signal_connect (cover->cover, "enter-event", G_CALLBACK (mouse_enter), cover->reflection);
    g_signal_connect (cover->cover, "leave-event", G_CALLBACK (mouse_leave), cover->reflection);
  }

  clutter_actor_set_rotation (group, CLUTTER_X_AXIS, -CAMERA_ANGLE, 0.0, COVER_WIDTH * 0.5, 0.0);

  clutter_container_add (CLUTTER_CONTAINER (stage), group, NULL);

  clutter_actor_show (stage);

  class = G_OBJECT_GET_CLASS (stage);
  pool = clutter_binding_pool_get_for_class (class);

  clutter_binding_pool_install_action (pool, "quit", CLUTTER_KEY_q, 0, G_CALLBACK (quit), NULL, NULL);
  clutter_binding_pool_install_action (pool, "left", CLUTTER_KEY_Left, 0, G_CALLBACK (left), coverflow, NULL);
  clutter_binding_pool_install_action (pool, "right", CLUTTER_KEY_Right, 0, G_CALLBACK (right), coverflow, NULL);

  g_signal_connect (stage, "key-press-event", G_CALLBACK (keyboard), stage);
  g_signal_connect (stage, "destroy", G_CALLBACK (clutter_main_quit), NULL);

  clutter_main ();

  return SUCCESS;
}
