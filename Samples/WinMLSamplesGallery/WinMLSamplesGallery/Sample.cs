using System.Collections.Generic;

namespace WinMLSamplesGallery
{
    public sealed class Sample
    {
        public string Title { get; set; }
        public string Description { get; set; }
        public string Icon { get; set; }
        public string Tag { get; set; }
        public string XAMLGithubLink { get; set; }
        public string CSharpGithubLink { get; set; }
        public List<SampleDoc> Docs { get; set; }
    }

    public sealed class SampleDoc
    {
        public string name { get; set; }
        public string link { get; set; }
    }
}
