class SnippetDataDictionary(dict):
    """
    Stores information for different snippets.
    The snippet index is used as a key to access the data.
    To be used in cases, where values might be added and removed across
    snippets. For every snippet index, where the topology is not changed and
    no new value is associated, the predecessor value is retrieved.
    """
    def __getitem__(self, snippet_index: int):
        """
        Get value to snippet index.
        If no value is present for the snippet index, the predecessor value
        is returned.
        :param snippet_index: Index of experiment snippet to get value for.
        """
        if snippet_index in self.keys():
            return super().get(snippet_index)
        if snippet_index < min(self.keys()):
            raise KeyError("Snippet index out of range.")
        return self[snippet_index - 1]

    def get(self, snippet_index: int):
        """
        Get value to snippet index.
        If no value is present for the snippet index, the predecessor value
        is returned.
        :param snippet_index: Index of experiment snippet to get value for.
        """
        return self[snippet_index]
