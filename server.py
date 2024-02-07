from game import settings

from kai.server.core import create_application

app = create_application(settings)

# Finally, run.
# SCREAMING NOTE: DO NOT RUN AS MULTI-PROCESS IT WILL FUCK *EVERYTHING* UP.
if __name__ == "__main__":
    app.run(
        host=app.ctx.settings.SERVER_INTERFACE,
        port=app.ctx.settings.SERVER_PORT,
        single_process=True,
        debug=settings.DEBUG,
        workers=0,
    )
