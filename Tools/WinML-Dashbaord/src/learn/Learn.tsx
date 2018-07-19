import { INavLink, Nav } from 'office-ui-fabric-react/lib/Nav';
import * as React from 'react';
import * as ReactMarkDown from 'react-markdown';

import './Docs.css';

interface IComponentState {
  markdownSrc: string,
}

export default class DocsView extends React.Component<{}, IComponentState> {
  constructor(props: {}) {
    super(props);
    this.state = {
      markdownSrc: '',
    };
  }

  public render() {
    return (
      <div id='DocsView'>
          <div id ='navpart'>
            <Nav
              groups={[
                {
                  links: [
                    {
                      isExpanded: true,
                      links: [
                        { name: 'Introduction', key: 'learn/01_intro.md', url: '', onClick: this.onClickHandler, },
                        { name: 'Convolutions', key: 'learn/02_intro_convolutions.md', url: '', onClick: this.onClickHandler, },
                        { name: 'Imaging Network Architectures', key: 'learn/03_modern_architectures.md', url: '', onClick: this.onClickHandler, },
                        { name: 'Model Comparison', key: 'learn/04_model_comparison.md', url: '', onClick: this.onClickHandler, }
                      ],
                      name: 'NN Study',
                      url: '',
                    }
                  ]
                }
              ]}
              />
          </div>
          <div id='markdownsource'>
            <ReactMarkDown source={this.state.markdownSrc} />
          </div>
      </div>
    );
  }

  private onClickHandler = (e: React.MouseEvent<HTMLElement>, item?: INavLink) => {
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
}
