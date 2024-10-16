 Certainly! I'll improve the code by leveraging code generation tools available for Node.js and enhancing the semantic structure of the generator. We'll use the following improvements:

1. Use TypeScript for better type safety and improved developer experience.
2. Implement Prettier for consistent code formatting.
3. Use ESLint for code quality and style enforcement.
4. Implement Jest for unit testing.
5. Use Plop.js for generating new widget types and templates.
6. Improve the semantic structure of the generator.

Let's start by updating the project structure:



<antArtifact identifier="improved-project-structure" type="application/vnd.ant.code" language="plaintext" title="Improved Project Structure">
esp32-watt-meter-designer/
├── src/
│   ├── templates/
│   │   ├── main.cpp.ejs
│   │   ├── Widget.h.ejs
│   │   └── Display.h.ejs
│   ├── lib/
│   │   ├── config-parser.ts
│   │   ├── code-generator.ts
│   │   ├── widget-factory.ts
│   │   └── file-manager.ts
│   ├── widgets/
│   │   ├── base-widget.ts
│   │   ├── text-widget.ts
│   │   ├── graph-widget.ts
│   │   └── gauge-widget.ts
│   ├── types/
│   │   ├── config.ts
│   │   └── widget.ts
│   ├── cli.ts
│   └── index.ts
├── configs/
│   └── default-config.json
├── callbacks/
│   ├── voltage-callbacks.ts
│   ├── current-callbacks.ts
│   └── power-callbacks.ts
├── generated/
│   └── (Arduino code will be generated here)
├── tests/
│   ├── config-parser.test.ts
│   ├── code-generator.test.ts
│   └── widget-factory.test.ts
├── plop-templates/
│   ├── widget.ts.hbs
│   └── callback.ts.hbs
├── package.json
├── tsconfig.json
├── .eslintrc.js
├── .prettierrc
├── jest.config.js
├── plopfile.js
├── README.md
└── .gitignore
</antArtifact>

Now, let's update the package.json file to include the new dependencies and scripts:



<antArtifact identifier="package-json" type="application/vnd.ant.code" language="json" title="package.json">
{
  "name": "esp32-watt-meter-designer",
  "version": "1.0.0",
  "description": "A TypeScript-based graphical designer and code generator for an Arduino ESP32 watt meter project",
  "main": "dist/index.js",
  "bin": {
    "esp32-watt-meter-designer": "./dist/cli.js"
  },
  "scripts": {
    "start": "ts-node src/cli.ts",
    "build": "tsc",
    "test": "jest",
    "lint": "eslint 'src/**/*.ts'",
    "format": "prettier --write 'src/**/*.ts'",
    "generate": "plop"
  },
  "keywords": [
    "esp32",
    "arduino",
    "watt-meter",
    "code-generator"
  ],
  "author": "Your Name",
  "license": "MIT",
  "dependencies": {
    "commander": "^8.3.0",
    "ejs": "^3.1.6",
    "chalk": "^4.1.2"
  },
  "devDependencies": {
    "@types/ejs": "^3.1.0",
    "@types/jest": "^27.4.0",
    "@types/node": "^16.11.7",
    "@typescript-eslint/eslint-plugin": "^5.3.1",
    "@typescript-eslint/parser": "^5.3.1",
    "eslint": "^8.2.0",
    "jest": "^27.3.1",
    "plop": "^3.0.5",
    "prettier": "^2.4.1",
    "ts-jest": "^27.0.7",
    "ts-node": "^10.4.0",
    "typescript": "^4.4.4"
  }
}
</antArtifact>

Now, let's create the TypeScript configuration file:



<antArtifact identifier="tsconfig-json" type="application/vnd.ant.code" language="json" title="tsconfig.json">
{
  "compilerOptions": {
    "target": "ES2018",
    "module": "commonjs",
    "lib": ["ES2018"],
    "outDir": "./dist",
    "rootDir": "./src",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "**/*.test.ts"]
}
</antArtifact>

Let's create the ESLint configuration file:



<antArtifact identifier="eslintrc-js" type="application/vnd.ant.code" language="javascript" title=".eslintrc.js">
module.exports = {
  parser: '@typescript-eslint/parser',
  extends: [
    'plugin:@typescript-eslint/recommended',
    'prettier/@typescript-eslint',
    'plugin:prettier/recommended',
  ],
  parserOptions: {
    ecmaVersion: 2018,
    sourceType: 'module',
  },
  rules: {
    // Custom rules here
  },
};
</antArtifact>

Now, let's create the Prettier configuration file:



<antArtifact identifier="prettierrc" type="application/vnd.ant.code" language="json" title=".prettierrc">
{
  "semi": true,
  "trailingComma": "all",
  "singleQuote": true,
  "printWidth": 100,
  "tabWidth": 2
}
</antArtifact>

Let's create the Jest configuration file:



<antArtifact identifier="jest-config-js" type="application/vnd.ant.code" language="javascript" title="jest.config.js">
module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  roots: ['<rootDir>/src'],
  testMatch: ['**/__tests__/**/*.ts', '**/?(*.)+(spec|test).ts'],
  moduleFileExtensions: ['ts', 'js', 'json', 'node'],
};
</antArtifact>

Now, let's create the Plop configuration file for generating new widgets and callbacks:



<antArtifact identifier="plopfile-js" type="application/vnd.ant.code" language="javascript" title="plopfile.js">
module.exports = function (plop) {
  plop.setGenerator('widget', {
    description: 'Create a new widget',
    prompts: [
      {
        type: 'input',
        name: 'name',
        message: 'What is the name of the widget?',
      },
    ],
    actions: [
      {
        type: 'add',
        path: 'src/widgets/{{kebabCase name}}-widget.ts',
        templateFile: 'plop-templates/widget.ts.hbs',
      },
    ],
  });

  plop.setGenerator('callback', {
    description: 'Create a new callback file',
    prompts: [
      {
        type: 'input',
        name: 'name',
        message: 'What is the name of the callback file?',
      },
    ],
    actions: [
      {
        type: 'add',
        path: 'callbacks/{{kebabCase name}}-callbacks.ts',
        templateFile: 'plop-templates/callback.ts.hbs',
      },
    ],
  });
};
</antArtifact>

Now, let's update some of the core files to leverage TypeScript and improve the semantic structure:

1. src/types/config.ts:



<antArtifact identifier="config-types" type="application/vnd.ant.code" language="typescript" title="src/types/config.ts">
export interface DisplayConfig {
  type: string;
  width: number;
  height: number;
  rotation?: number;
  backgroundColor?: string;
}

export interface ProjectConfig {
  name: string;
  display: DisplayConfig;
}

export interface WidgetConfig {
  type: string;
  name: string;
  x: number;
  y: number;
  width: number;
  height: number;
  processCallback: string;
  displayCallback: string;
  [key: string]: any;
}

export interface Config {
  project: ProjectConfig;
  widgets: WidgetConfig[];
}
</antArtifact>

2. src/types/widget.ts:



<antArtifact identifier="widget-types" type="application/vnd.ant.code" language="typescript" title="src/types/widget.ts">
import { WidgetConfig } from './config';

export interface Widget {
  config: WidgetConfig;
  generateSetup(): string;
  generateDeclaration(): string;
  generateProcessCall(): string;
  generateDisplayCall(): string;
}

export interface WidgetConstructor {
  new (config: WidgetConfig): Widget;
}
</antArtifact>

3. src/lib/config-parser.ts:



<antArtifact identifier="config-parser" type="application/vnd.ant.code" language="typescript" title="src/lib/config-parser.ts">
import { Config, WidgetConfig } from '../types/config';
import { Widget } from '../types/widget';
import WidgetFactory from './widget-factory';

export default class ConfigParser {
  private config: Config;
  private widgetFactory: WidgetFactory;

  constructor(config: Config) {
    this.config = config;
    this.widgetFactory = new WidgetFactory();
    this.validate();
  }

  private validate(): void {
    if (!this.config.project || !this.config.project.name) {
      throw new Error('Project name is required');
    }

    if (!this.config.project.display || !this.config.project.display.type) {
      throw new Error('Display type is required');
    }

    if (!Array.isArray(this.config.widgets) || this.config.widgets.length === 0) {
      throw new Error('At least one widget is required');
    }

    this.config.widgets = this.config.widgets.map(this.validateAndCreateWidget.bind(this));
  }

  private validateAndCreateWidget(widgetConfig: WidgetConfig, index: number): Widget {
    const requiredFields = ['type', 'name', 'x', 'y', 'width', 'height', 'processCallback', 'displayCallback'];
    requiredFields.forEach(field => {
      if (!(field in widgetConfig)) {
        throw new Error(`Widget ${index} is missing required field: ${field}`);
      }
    });

    if (typeof widgetConfig.x !== 'number' || typeof widgetConfig.y !== 'number' ||
        typeof widgetConfig.width !== 'number' || typeof widgetConfig.height !== 'number') {
      throw new Error(`Widget ${index} has invalid position or dimensions`);
    }

    return this.widgetFactory.createWidget(widgetConfig);
  }

  public injectCallbacks(callbacks: Record<string, string>): void {
    this.config.widgets.forEach(widget => {
      (widget as any).processCallbackCode = callbacks[widget.config.processCallback];
      (widget as any).displayCallbackCode = callbacks[widget.config.displayCallback];
    });
  }

  public get project(): Config['project'] {
    return this.config.project;
  }

  public get widgets(): Widget[] {
    return this.config.widgets as Widget[];
  }
}
</antArtifact>

These improvements leverage TypeScript for better type safety, use ESLint and Prettier for code quality and formatting, set up Jest for testing, and use Plop.js for generating new widget types and templates. The semantic structure of the generator has been improved by using interfaces and more strongly typed classes.

To complete the project, you would need to update the remaining files to use TypeScript and implement the testing suite. The improved structure and tooling will make the project more maintainable, extensible, and robust.
