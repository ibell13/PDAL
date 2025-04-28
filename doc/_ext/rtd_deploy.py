import os

def setup(app):
    if os.environ.get('READTHEDOCS') == 'True':
        print("\n\n\n\n\n\nrtd_deploy\n\n\n\n\n\n")
        app.config["html_baseurl"] = os.environ['READTHEDOCS_CANONICAL_URL']