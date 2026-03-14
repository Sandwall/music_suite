# music_suite

# planning

## renderer
we'll keep a track of all of the objects that have been drawn each frame
this is so that we can slap a z coordinate on every image we've drawn

but we'll also store quads and text in two different buffers
this is so that transparency works when rendering text

you render all of the quads with their depth,
then render all of the text with their depth.
and then boom! the text should be obscured by the quads properly

but if you want to render any custom image, we'll first render everything we can
(this inclused text and quads, we're doing this to "clear" the context)

and then switch the current texture to whatever image we want to render,
add a single quad