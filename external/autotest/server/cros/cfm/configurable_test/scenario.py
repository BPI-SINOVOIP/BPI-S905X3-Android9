class Scenario(object):
    """
    A scenario with a number of actions to perform.
    """
    def __init__(self, *args):
        self.actions = args

    def execute(self, context):
        """
        Executes the scenario.

        @param context ActionContext instance providing the dependencies for
                the actions in the scenario.
        """
        for action in self.actions:
            action.execute(context)

    def __repr__(self):
        return 'Scenario [actions=%s]' % str(self.actions)

