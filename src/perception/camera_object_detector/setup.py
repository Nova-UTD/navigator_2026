from setuptools import find_packages, setup

package_name = 'camera_object_detector'

setup(
    name=package_name,
    version='1.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='root',
    maintainer_email='project.nova@utdallas.edu',
    description='Detects cars, people and bikes in camera images',
    license='MIT License',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'camera_object_detector = camera_object_detector.camera_object_detector:main'
        ],
    },
)
