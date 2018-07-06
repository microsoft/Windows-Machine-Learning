import * as React from 'react';
import * as ReactMarkDown from 'react-markdown';

import { Nav, INavLink } from 'office-ui-fabric-react/lib/Nav';
//import SplitPane from 'react-split-pane';
import './Docs.css';


type ComponentProperties = {}

type ComponentState = {
  markdownSrc: string,
}

export default class DocsView extends React.Component<ComponentProperties, ComponentState> {
  private tab: React.RefObject<HTMLDivElement>;

  private selected: string;
  constructor(props: {}) {
    super(props);
    this.state = {
      markdownSrc: '',
    };

    this.tab = React.createRef();

    this._onIntroductionClickHandler = this._onIntroductionClickHandler.bind(this);

  }

  render() {
    return (
      <div id='DocsView' ref={this.tab}>
          <div id ='navpart'>
            <Nav selectedKey={this.selected}
              groups={[
                {
                  links: [
                    {
                      name: 'NN Study', url: '', isExpanded: true,

                      links: [
                        { name: 'Introduction', key: './content/01_intro.md', url: '', onClick: this._onIntroductionClickHandler, },
                        { name: 'Convolutions', key: './content/02_intro_convolutions.md', url: '', onClick: this._onIntroductionClickHandler, },
                        { name: 'Imaging Network Architectures', key: './content/03_modern_architectures.md', url: '', onClick: this._onIntroductionClickHandler, },
                        { name: 'Model Comparison', key: './content/04_model_comparison.md', url: '', onClick: this._onIntroductionClickHandler, }
                      ]
                    }
                  ]
                }
              ]}
            />
          </div>
          <div id='markdownsource'>
            <ReactMarkDown source={this.state.markdownSrc}
            />
          </div>


      </div>
    );
  }
  private _onIntroductionClickHandler(e: React.MouseEvent<HTMLElement>, item?: INavLink): false {
    if (item) {
      fetch(item.key)
        .then(response => response.text())
        .then((filecontents) => {
          this.setState({
            markdownSrc: filecontents,
          });
        });
    }
    return false;
  }

  componentDidMount() {
    this.tab.current && this.tab.current.scrollIntoView({
      behavior: 'smooth',
    });
  }
}
